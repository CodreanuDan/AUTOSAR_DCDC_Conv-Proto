/*
 * Filename: DcmSf.c
 * @brief: Contains DSL, DSD, and DSP implementation for Diagnostic Manager (DcmSf)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "DcmSf.h"
#include "UartSf.h"
#include "DemSf.h"
#include "PwmSf.h"
#include "DioSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/**
 * @brief Data type classification for generic DID payload processing
 */
typedef enum {
    DID_DATA_UINT8,       /* 1-byte raw value (uint8_t, bool, status flag) */
    DID_DATA_UINT16,      /* 2-byte numeric value (uint16_t) */
    DID_DATA_ARRAY        /* Multi-byte array payload (e.g., [DutyA, DutyB]) */
} Dcm_DidDataType;

/**
 * @brief DID Access Rights
 */
typedef enum {
    DID_READ_ONLY,
    DID_READ_WRITE
} Dcm_DidAccessType;

/**
 * @brief Pointer type for optional hardware/RTE update notification callbacks
 */
typedef void (*Dcm_DidUpdateNotificationFctPtr)(void);

/**
 * @brief Central Data Identifier Configuration Structure
 */
typedef struct {
    uint16_t                        did;            /* 2-byte DID identifier (e.g., 0x0100) */
    uint8_t                         data_size;      /* Payload size in bytes (1 or 2) */
    Dcm_DidDataType                 data_type;      /* Payload data layout type */
    Dcm_DidAccessType               access_type;    /* Read / Write access permission */
    void                           *p_data;         /* Pointer to underlying global variable/memory */
    Dcm_DidUpdateNotificationFctPtr fct_update;     /* Optional callback triggered on successful write */
} Dcm_DidConfigType;

/* =====================================================================
 * LOCAL CALLBACK PROTOTYPES FOR HARDWARE UPDATES
 * ===================================================================== */
static void Dcm_Cb_UpdatePwmDuty(void)  { Pwm_SetDutyCycle(g_duty_a, g_duty_b); }
static void Dcm_Cb_UpdatePwmFreq(void)  { Pwm_SetFrequency(g_target_frequency); }
static void Dcm_Cb_UpdateRelayIn(void)  { (void)Rte_Call_RpRelayControl_SetRelayState(DIO_CHANNEL_RELAY_IN, (Dio_LevelType)g_relay_input_state); }
static void Dcm_Cb_UpdateRelayOut(void) { (void)Rte_Call_RpRelayControl_SetRelayState(DIO_CHANNEL_RELAY_OUT, (Dio_LevelType)g_relay_output_state); }

/* =====================================================================
 * CENTRAL DID CONFIGURATION TABLE 
 * ===================================================================== */
static const Dcm_DidConfigType s_dcm_did_table[] = {
    /* DID     Size Type             Access          Data Pointer                   Hardware Notification Callback */
    { 0x0100U, 2U,  DID_DATA_ARRAY,  DID_READ_WRITE, (void*)&g_duty_a,              Dcm_Cb_UpdatePwmDuty },
    { 0x0101U, 2U,  DID_DATA_UINT16, DID_READ_WRITE, (void*)&g_target_frequency,    Dcm_Cb_UpdatePwmFreq },
    { 0x0102U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_target_vout,         NULL },
    { 0x0103U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_pid_disable_flag,    NULL },
    { 0x0104U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_relay_input_state,   Dcm_Cb_UpdateRelayIn },
    { 0x0105U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_relay_output_state,  Dcm_Cb_UpdateRelayOut },
    { 0x0106U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_cyclic_fault_updates,NULL },
    { 0x0107U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_cyclic_pid_updates,  NULL },
    { 0x0108U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_cyclic_pwm_updates,  NULL },
    { 0x0109U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_cyclic_act_updates,  NULL },
    { 0x0110U, 1U,  DID_DATA_UINT8,  DID_READ_WRITE, (void*)&g_cyclic_conv_updates, NULL }
};

#define DCM_TOTAL_DIDS (sizeof(s_dcm_did_table) / sizeof(Dcm_DidConfigType))

/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *               Local Function Prototypes
 *******************************************************/

/* Helper TX functions */
static void Dcm_SendPositiveResponse(uint8_t sid, const uint8_t *data_ptr, uint8_t len);
static void Dcm_SendNegativeResponse(uint8_t sid, uint8_t nrc);

/* DSP Service Processors */
static void Dcm_Dsp_EcuReset(const uint8_t *req_ptr);
static void Dcm_Dsp_ClearFaultMem(void);
static void Dcm_Dsp_ReadDtcInfo(const uint8_t *req_ptr);
static void Dcm_Dsp_ReadDataByIdentifier(const uint8_t *req_ptr);
static void Dcm_Dsp_WriteDataByIdentifier(const uint8_t *req_ptr);
static void Dcm_Dsp_RoutineControl(const uint8_t *req_ptr);

/************* End of Local Function Prototypes *********/

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

void Dcm_Init(void)
{
    s_current_session = DCM_DEFAULT_SESSION;
}

Dcm_SesCtrlType Dcm_GetSesCtrlType(void)
{
    return s_current_session;
}

/* =====================================================================
 * DSD (Diagnostic Service Dispatcher) LAYER
 * ===================================================================== */

void Dcm_ProcessRxPdu(const uint8_t *pdu_ptr)
{
    if (pdu_ptr == NULL)
    {
        return;
    }

    uint8_t sid = pdu_ptr[0];

    /* Route request to specific DSP processor */
    switch (sid)
    {
        case UDS_SID_ECU_RESET:
            Dcm_Dsp_EcuReset(pdu_ptr);
            break;
        case UDS_SID_CLEAR_FAULT_MEM:
            Dcm_Dsp_ClearFaultMem();
            break;
        case UDS_SID_READ_DTC_INFO:
            Dcm_Dsp_ReadDtcInfo(pdu_ptr);
            break;
        case UDS_SID_READ_DATA_BY_ID:
            Dcm_Dsp_ReadDataByIdentifier(pdu_ptr);
            break;
        case UDS_SID_WRITE_DATA_BY_ID:
            Dcm_Dsp_WriteDataByIdentifier(pdu_ptr);
            break;
        case UDS_SID_ROUTINE_CONTROL:
            Dcm_Dsp_RoutineControl(pdu_ptr);
            break;
        default:
            Dcm_SendNegativeResponse(sid, UDS_NRC_SERVICE_NOT_SUPPORTED);
            break;
    }
}

/* =====================================================================
 * DSP (Diagnostic Service Processing) LAYER
 * ===================================================================== */

static void Dcm_Dsp_EcuReset(const uint8_t *req_ptr)
{
    uint8_t reset_type = req_ptr[1];

    if (reset_type == 0x02U) /* Soft Reset */
    {
        /* Send positive response first before triggering state transition */
        Dcm_SendPositiveResponse(UDS_SID_ECU_RESET, &reset_type, 1U);

        /* Set flag or notify EcuM to transition into SHUTDOWN/RESET state */
        /* (EcuM will invoke NvM_WriteAll and Wdg_PerformReset) */
    }
    else
    {
        Dcm_SendNegativeResponse(UDS_SID_ECU_RESET, UDS_NRC_SUB_FUNCTION_NOT_SUPP);
    }
}

static void Dcm_Dsp_ClearDiagInfo(void)
{
    /* Clear fault memory using DemSf */
    Dem_ClearDiagnosticInformation();
    
    Dcm_SendPositiveResponse(UDS_SID_CLEAR_DIAG_INFO, NULL, 0U);
}

static void Dcm_Dsp_ReadDtcInfo(const uint8_t *req_ptr)
{
    uint8_t sub_func = req_ptr[1];

    if (sub_func == 0x02U) /* reportDTCByStatusMask */
    {
        /* Handled by DemSf multi-frame response */
        Dem_ReportDTCByStatusMask();
    }
    else
    {
        Dcm_SendNegativeResponse(UDS_SID_READ_DTC_INFO, UDS_NRC_SUB_FUNCTION_NOT_SUPP);
    }
}

/**
 * Function name: Dcm_Dsp_ReadDataByIdentifier
 * @brief Generic Handler for UDS Service 0x22 (Read Data By Identifier)
 * Searches the DID configuration table and packs the response payload dynamically.
 * @param: const uint8_t* req_ptr (Pointer to incoming UDS request frame)
 * @return: void
 */
static void Dcm_Dsp_ReadDataByIdentifier(const uint8_t *req_ptr)
{
    uint16_t requested_did = (uint16_t)(((uint16_t)req_ptr[1] << 8U) | req_ptr[2]);
    bool did_found = false;

    for (uint8_t i = 0U; i < DCM_TOTAL_DIDS; i++)
    {
        if (s_dcm_did_table[i].did == requested_did)
        {
            uint8_t resp_payload[8];

            /* Echo back the requested DID in response header bytes 0 and 1 */
            resp_payload[0] = req_ptr[1];
            resp_payload[1] = req_ptr[2];

            /* Generic data extraction based on configured layout type */
            if (s_dcm_did_table[i].data_type == DID_DATA_UINT16)
            {
                uint16_t val = *((uint16_t*)s_dcm_did_table[i].p_data);
                resp_payload[2] = (uint8_t)(val & 0xFFU);          /* Low Byte */
                resp_payload[3] = (uint8_t)((val >> 8U) & 0xFFU);  /* High Byte */
            }
            else /* DID_DATA_UINT8 or DID_DATA_ARRAY */
            {
                /* Direct memory byte-by-byte copy */
                uint8_t *p_src = (uint8_t*)s_dcm_did_table[i].p_data;
                for (uint8_t b = 0U; b < s_dcm_did_table[i].data_size; b++)
                {
                    resp_payload[2U + b] = p_src[b];
                }
            }

            /* Transmit UDS Positive Response (0x62) with total payload size */
            Dcm_SendPositiveResponse(UDS_SID_READ_DATA_BY_ID, resp_payload, s_dcm_did_table[i].data_size + 2U);
            did_found = true;
            break;
        }
    }
    if (!did_found)
    {
        /* DID not configured in table: Return NRC 0x31 (Request Out Of Range) */
        Dcm_SendNegativeResponse(UDS_SID_READ_DATA_BY_ID, UDS_NRC_REQUEST_OUT_OF_RANGE);
    }
}

/**
 * Function name: Dcm_Dsp_WriteDataByIdentifier
 * @brief Generic Handler for UDS Service 0x2E (Write Data By Identifier)
 * Updates memory dynamically and triggers optional hardware/RTE notification callbacks.
 * @param: const uint8_t* req_ptr (Pointer to incoming UDS request frame)
 * @return: void
 */
static void Dcm_Dsp_WriteDataByIdentifier(const uint8_t *req_ptr)
{
    uint16_t requested_did = (uint16_t)(((uint16_t)req_ptr[1] << 8U) | req_ptr[2]);
    bool did_found = false;

    for (uint8_t i = 0U; i < DCM_TOTAL_DIDS; i++)
    {
        if (s_dcm_did_table[i].did == requested_did)
        {
            /* Check read/write access rights */
            if (s_dcm_did_table[i].access_type != DID_READ_WRITE)
            {
                Dcm_SendNegativeResponse(UDS_SID_WRITE_DATA_BY_ID, UDS_NRC_CONDITIONS_NOT_CORRECT);
                return;
            }
        }
        /* 1. Generic memory update based on data layout type */
        if (s_dcm_did_table[i].data_type == DID_DATA_UINT16)
        {
            uint16_t val = (uint16_t)(((uint16_t)req_ptr[4] << 8U) | req_ptr[3]);
            *((uint16_t*)s_dcm_did_table[i].p_data) = val;
        }
        else /* DID_DATA_UINT8 or DID_DATA_ARRAY */
        {
            uint8_t *p_dest = (uint8_t*)s_dcm_did_table[i].p_data;
            for (uint8_t b = 0U; b < s_dcm_did_table[i].data_size; b++)
            {
                p_dest[b] = req_ptr[3U + b];
            }
        }
        /* 2. Execute hardware/RTE update notification callback if configured */
        if (s_dcm_did_table[i].fct_update != NULL)
        {
            s_dcm_did_table[i].fct_update();
        }
        /* 3. Transmit UDS Positive Response (0x6E) echoing back confirmed DID */
        uint8_t resp_payload[2] = { req_ptr[1], req_ptr[2] };
        Dcm_SendPositiveResponse(UDS_SID_WRITE_DATA_BY_ID, resp_payload, 2U);

        did_found = true;
        break;
    }
    if (!did_found)
    {
        /* DID not configured in table: Return NRC 0x31 (Request Out Of Range) */
        Dcm_SendNegativeResponse(UDS_SID_WRITE_DATA_BY_ID, UDS_NRC_REQUEST_OUT_OF_RANGE);
    }
}


/* =====================================================================
 * HELPERS: POSITIVE & NEGATIVE RESPONSES
 * ===================================================================== */

static void Dcm_SendPositiveResponse(uint8_t sid, const uint8_t *data_ptr, uint8_t len)
{
    uint8_t checksum = 0U;
    uint8_t pos_sid = sid + 0x40U;

    Uart_TxByte(pos_sid);
    checksum += pos_sid;

    for (uint8_t i = 0U; i < len; i++)
    {
        Uart_TxByte(data_ptr[i]);
        checksum += data_ptr[i];
    }

    /* Fill padding up to 10 bytes payload */
    for (uint8_t i = len; i < 9U; i++)
    {
        Uart_TxByte(0x00U);
        checksum += 0x00U;
    }

    Uart_TxByte(checksum);
    Uart_TxByte(0x0DU);
}

static void Dcm_SendNegativeResponse(uint8_t sid, uint8_t nrc)
{
    uint8_t checksum = 0U;

    Uart_TxByte(0x7FU);       /* UDS Negative Response Header */
    checksum += 0x7FU;

    Uart_TxByte(sid);         /* Requested SID */
    checksum += sid;

    Uart_TxByte(nrc);         /* NRC code */
    checksum += nrc;

    /* Fill padding up to 10 bytes payload */
    for (uint8_t i = 0U; i < 7U; i++)
    {
        Uart_TxByte(0x00U);
        checksum += 0x00U;
    }

    Uart_TxByte(checksum);
    Uart_TxByte(0x0DU);
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of DcmSf.c */