/*
 * Filename: DemSf.c
 * @brief: Contains function definitions and DTC fault table for Diagnostic Event Manager (DemSf)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "DemSf.h"
#include "AdcSf.h"
#include "ComSf.h"
#include "Rte.h"
#include "CoolingEvents.h"
#include "SafetyEvents.h"
/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/* Central Diagnostic Trouble Code (DTC) Fault Table */
DTC_StatusType g_dtc_table[] = {
    { 0x1A0100U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_IIN,  	  DemSf_Monitor_ElectricalFaults, SHORT_TO_VBAT },          /* Iin Short to VBAT */
    { 0x1A0101U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VIN,  	  DemSf_Monitor_ElectricalFaults, SHORT_TO_VBAT },          /* Vin Short to VBAT */
    { 0x1A0102U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_IOUT, 	  DemSf_Monitor_ElectricalFaults, SHORT_TO_VBAT },          /* Iout Short to VBAT */
    { 0x1A0103U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VOUT, 	  DemSf_Monitor_ElectricalFaults, SHORT_TO_VBAT },          /* Vout Short to VBAT */
    { 0x1A0110U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_IIN,  	  DemSf_Monitor_ElectricalFaults, SHORT_TO_GND },           /* Iin Short to GND */
    { 0x1A0111U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VIN,  	  DemSf_Monitor_ElectricalFaults, SHORT_TO_GND },           /* Vin Short to GND */
    { 0x1A0112U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_IOUT, 	  DemSf_Monitor_ElectricalFaults, SHORT_TO_GND },           /* Iout Short to GND */
    { 0x1A0113U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VOUT, 	  DemSf_Monitor_ElectricalFaults, SHORT_TO_GND },           /* Vout Short to GND */
    { 0x1A0120U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_IIN,  	  DemSf_Monitor_ElectricalFaults, OPEN_CIRCUIT_CURRENT },   /* Iin Open Circuit */
    { 0x1A0121U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VIN,  	  DemSf_Monitor_ElectricalFaults, OPEN_CIRCUIT_VOLTAGE },   /* Vin Open Circuit */
    { 0x1A0122U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_IOUT, 	  DemSf_Monitor_ElectricalFaults, OPEN_CIRCUIT_CURRENT },   /* Iout Open Circuit */
    { 0x1A0123U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VOUT, 	  DemSf_Monitor_ElectricalFaults, OPEN_CIRCUIT_VOLTAGE },   /* Vout Open Circuit */
    { 0x1A0140U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_TEMP,  DemSf_Monitor_Cooling, 		  COOL_EVT_SHT_COMM },
    { 0x1A0141U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_TEMP,  DemSf_Monitor_Cooling, 		  COOL_EVT_SHT_RANGE },
    { 0x1A0150U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_LEVEL, DemSf_Monitor_Cooling, 		  COOL_EVT_LVL_LOW },
    { 0x1A0151U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_LEVEL, DemSf_Monitor_Cooling, 		  COOL_EVT_LVL_EMPTY },
    { 0x1A0152U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_LEVEL, DemSf_Monitor_Cooling, 		  COOL_EVT_LVL_SENSOR },
    { 0x1A0160U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_TEMP,  DemSf_Monitor_Cooling, 		  COOL_EVT_OVERTEMP_W },
    { 0x1A0161U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_TEMP,  DemSf_Monitor_Cooling, 		  COOL_EVT_OVERTEMP_C },
    { 0x1A0162U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, DEM_FF_SRC_FAN,   DemSf_Monitor_Cooling, 		  COOL_EVT_INEFFECTIVE },
    { 0x1A0170U, DEM_STATUS_OK, DTC_STATUS_MISSING, 0U, 0U, ADC_CH_VIN,       DemSf_Monitor_Safety,           SAFE_CAUSE_FUSE_BLOWN }
};

/* Dynamic calculation of total registered DTCs */
#define TOTAL_DTCS (sizeof(g_dtc_table) / sizeof(g_dtc_table[0]))

/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *               Local Function Prototypes
 *******************************************************/
static void Dem_UpdateEvent(DTC_StatusType *dtc, uint8_t error_condition);
static uint16_t Dem_GetFreezeValue(uint8_t src);

/************* End of Local Function Prototypes *********/


/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

 void Dem_Init(void)
{
    /* Initialize default fault state flags */
	uint8_t i;
    for (i = 0U; i < TOTAL_DTCS; i++)
    {
        g_dtc_table[i].qualifying_status = DEM_STATUS_OK;
        g_dtc_table[i].fault_status      = DTC_STATUS_MISSING;
        g_dtc_table[i].debounce_cnt      = 0U;
        g_dtc_table[i].freeze_frame_data = 0U;
    }
}

/**
 * Function name: DemSf_Monitor_ElectricalFaults
 * @brief Evaluates electrical raw ADC counts against defined threshold boundaries.
 * @param: uint8_t param (ADC Channel ID)
 * @param: uint8_t mode (Fault condition mode: SHORT_TO_GND, SHORT_TO_VBAT, OPEN_CIRCUIT)
 * @return: uint8_t (1 if fault condition is present, 0 otherwise)
 */
uint8_t DemSf_Monitor_ElectricalFaults(uint8_t param, uint8_t mode)
{
    /* Read ADC raw buffer data and populate local buffer */
    uint16_t raw_adc_buffer[ADC_NUM_CHANNELS];
    Adc_ReadGroup(raw_adc_buffer);

    if (mode == SHORT_TO_GND)
    {
        if (raw_adc_buffer[param] == 0U) 
        { 
            return 1U; 
        }
    }
    else if (mode == SHORT_TO_VBAT)
    {
        if (raw_adc_buffer[param] == 1023U) 
        { 
            return 1U; 
        }
    }
    else if (mode == OPEN_CIRCUIT_VOLTAGE)
    {
        if ((raw_adc_buffer[param] > 810U) && (raw_adc_buffer[param] != 1023U)) 
        { 
            return 1U; 
        }
    }
    else if (mode == OPEN_CIRCUIT_CURRENT)
    {
        if ((raw_adc_buffer[param] < 400U) && (raw_adc_buffer[param] != 0U)) 
        { 
            return 1U; 
        }
    }
    
    return 0U;
}

/* param = freeze source (unused here), mode = COOL_EVT_* bit to test */
uint8_t DemSf_Monitor_Cooling(uint8_t param, uint8_t mode)
{
	(void)param;
	return ((Rte_Read_CoolFaults() & mode) != 0U) ? 1U : 0U;
}

uint8_t DemSf_Monitor_Safety(uint8_t param, uint8_t mode)
{
    (void)param;
    return ((Rte_Read_SafetyCauses() & mode) != 0U) ? 1U : 0U;
}

/* Helper function to combine freeze frame data from adc and from cooling faults */
static uint16_t Dem_GetFreezeValue(uint8_t src)
{
    uint16_t raw_adc_buffer[ADC_NUM_CHANNELS];

    if (src < ADC_NUM_CHANNELS)
    {
        Adc_ReadGroup(raw_adc_buffer);
        return raw_adc_buffer[src];
    }
    switch (src)
    {
        case DEM_FF_SRC_TEMP:  return (uint16_t)Rte_Read_AmbTemp();
        case DEM_FF_SRC_LEVEL: return (uint16_t)Rte_Read_CoolantLevelRaw();
        case DEM_FF_SRC_FAN:   return (uint16_t)Rte_Read_FanDuty();
        default:               return 0U;
    }
}

/**
 * Function name: Dem_UpdateEvent
 * @brief Internal debouncing integrator and status transition state machine.
 * @param: DTC_StatusType *dtc (Pointer to target DTC event record)
 * @param: uint8_t error_condition (Evaluation result from monitor function: 1=Failed, 0=Passed)
 * @return: void
 */
static void Dem_UpdateEvent(DTC_StatusType *dtc, uint8_t error_condition)
{
    if (dtc == NULL)
    {
        return;
    }

    /* CASE 1: Error condition is active and persisting */
    if (error_condition != 0U)
    {
        /* Increment debouncing counter up to max threshold */
        if (dtc->debounce_cnt < DEBOUNCE_THRESHOLD) 
        {
            dtc->debounce_cnt++;
        }

        /* Transition from OK to PENDING on first error detection */
        if (dtc->qualifying_status == DEM_STATUS_OK) 
        {
            dtc->qualifying_status = DEM_STATUS_PENDING;
        }

        /* Qualify fault as CONFIRMED once debouncing counter exceeds threshold */
        if ((dtc->qualifying_status == DEM_STATUS_PENDING) && (dtc->debounce_cnt >= DEBOUNCE_THRESHOLD))
        {
            dtc->qualifying_status = DEM_STATUS_CONFIRMED;
            dtc->fault_status = DTC_STATUS_ACTIVE;
            
			/* Fetch live ADC raw sample or cooling data for freeze frame data */
            uint16_t raw_adc_buffer[ADC_NUM_CHANNELS];
            Adc_ReadGroup(raw_adc_buffer);
            dtc->freeze_frame_data = Dem_GetFreezeValue(dtc->adc_source_ptr);
        }
    }
    /* CASE 2: Error condition is not active*/
    else
    {
        /* Decrement debouncing counter down to 0 */
        if (dtc->debounce_cnt > 0U) 
        {
            dtc->debounce_cnt--; 
        }
        else 
        {
            /* Error condition cleared and debounce counter reaches 0 */
            if (dtc->qualifying_status == DEM_STATUS_CONFIRMED)
            {
                /* Previously confirmed fault becomes PASSIVE until cleared via UDS 0x14 */
                dtc->qualifying_status = DEM_STATUS_OK;
                dtc->fault_status = DTC_STATUS_PASSIVE; 
            }
            else if (dtc->qualifying_status == DEM_STATUS_PENDING)
            {
                /* Unconfirmed pending fault disappears completely */
                dtc->qualifying_status = DEM_STATUS_OK;
            }
        }
    }
}

/**
 * Function name: DemSf_MainFunction
 * @brief Cyclic task (e.g., 10ms) that evaluates fault monitoring functions and updates debouncing counters.
 * @param: void
 * @return: void
 */
void DemSf_MainFunction(void) 
{
	uint8_t i;
    for (i = 0U; i < TOTAL_DTCS; i++) 
    {
        uint8_t error_detected = 0U;
        
		if (g_dtc_table[i].monitor_func != NULL)
        {
            error_detected = g_dtc_table[i].monitor_func((g_dtc_table[i].adc_source_ptr), g_dtc_table[i].error_mode);
        }
        
        Dem_UpdateEvent(&g_dtc_table[i], error_detected);
    }
}

/**
 * Function name: Dem_ClearDiagnosticInformation
 * @brief Clears confirmed and passive DTCs from fault memory (Triggered by UDS Service 0x14).
 * @param: void
 * @return: void
 */
void Dem_ClearDiagnosticInformation(void)
{
	uint8_t i;
    for (i = 0U; i < TOTAL_DTCS; i++)
    {
        g_dtc_table[i].qualifying_status = DEM_STATUS_OK;
        g_dtc_table[i].fault_status      = DTC_STATUS_MISSING;
        g_dtc_table[i].debounce_cnt      = 0U;
        g_dtc_table[i].freeze_frame_data = 0U;
    }
}

/**
 * Function name: Dem_ReportDTCByStatusMask
 * @brief Triggers multi-frame active/passive DTC payload transmission via ComSf (Triggered by UDS Service 0x19).
 * @param: void
 * @return: void
 */
void Dem_ReportDTCByStatusMask(void)
{
    /* Invoke ComSf multi-frame transmission handler */
    Com_Send_ActiveDTC_Frames();
}

/**
 * Function name: Dem_GetTotalDtcs
 * @brief Returns the dynamically calculated number of registered DTCs in the system.
 * @param: void
 * @return: uint8_t Count of DTCs in table
 */
uint8_t Dem_GetTotalDtcs(void)
{
    return (uint8_t)TOTAL_DTCS;
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of DemSf.c */