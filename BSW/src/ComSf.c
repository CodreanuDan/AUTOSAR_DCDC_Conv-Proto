/*
 * Filename: ComSf.c
 * @brief: Contains function definitions for RX Parsing in Communication Stack (ComSf)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */


/* Core libs */
#include <avr/io.h>

/* Project specific libs */
#include "ComSf.h"
#include "UartSf.h"
#include "PduR.h"
#include "AdcSf.h"
#include "DemSf.h"
#include "PwmSf.h"
#include "Rte.h"
#include "DioSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/* Alive Counters for frame sequence tracking */
static uint8_t s_alive_counter_DiagFrame_ConvMonitorData = 0U;
static uint8_t s_alive_counter_DiagFrame_PWMInfo         = 0U;
static uint8_t s_alive_counter_DiagFrame_PID             = 0U;
static uint8_t s_alive_counter_DiagFrame_Act             = 0U;
static uint8_t s_alive_counter_DiagFrame_DTC             = 0U;

/* Static Cyclic Transmission Flags (Controlled via Setters) */
static uint8_t s_cyclic_conv_updates  = 1U; /* DID 0x0110 CycCnvUpdt: Default enabled */
static uint8_t s_cyclic_pwm_updates   = 1U; /* DID 0x0108 CycPwmUpdt: Default enabled */
static uint8_t s_cyclic_pid_updates   = 1U; /* DID 0x0107 CycPidUpdt: Default enabled */
static uint8_t s_cyclic_act_updates   = 1U; /* DID 0x0109 CycActUpdt: Default enabled */
static uint8_t s_cyclic_fault_updates = 1U; /* DID 0x0106 CycDemUpdt: Default enabled */


/*******************************************************
 *      CYCLIC CONTROL SETTERS & GETTERS FOR DCM
 *******************************************************/
void Com_SetCyclicConvUpdates(uint8_t enable)  { s_cyclic_conv_updates = enable; }
uint8_t Com_GetCyclicConvUpdates(void)          { return s_cyclic_conv_updates; }

void Com_SetCyclicPwmUpdates(uint8_t enable)   { s_cyclic_pwm_updates = enable; }
uint8_t Com_GetCyclicPwmUpdates(void)           { return s_cyclic_pwm_updates; }

void Com_SetCyclicPidUpdates(uint8_t enable)   { s_cyclic_pid_updates = enable; }
uint8_t Com_GetCyclicPidUpdates(void)           { return s_cyclic_pid_updates; }

void Com_SetCyclicActUpdates(uint8_t enable)   { s_cyclic_act_updates = enable; }
uint8_t Com_GetCyclicActUpdates(void)           { return s_cyclic_act_updates; }

void Com_SetCyclicFaultUpdates(uint8_t enable) { s_cyclic_fault_updates = enable; }
uint8_t Com_GetCyclicFaultUpdates(void)         { return s_cyclic_fault_updates; }

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: Com_MainFunction_Rx
 * @brief Periodically polls the UART ring buffer, parses incoming bytes,
 * validates frame checksums, and routes valid frames to DCM.
 * @param: void
 * @return: void
 */
void Com_MainFunction_Rx(void)
{
	uint8_t i_1;
	uint8_t i_2;
	uint8_t i_3;

    while (1)
    {
        /* Obtain bytes available in UART circular buffer */
        uint16_t bytes_available = Uart_GetRxBytesAvailable();

        /* Break loop until a complete frame of 12 bytes is available */
        if (bytes_available < COM_RX_FRAME_SIZE)
        {
            break;
        }

        /* Peek at frame terminator index (Offset 11) */
        uint8_t terminator_index = Uart_GetRxPeekIndex(COM_RX_FRAME_SIZE - 1U);

        if (Uart_GetRxBufferByte(terminator_index) == 0x0DU)
        {
            uint8_t rx_frame[COM_RX_FRAME_SIZE];

            /* Copy full 12-byte payload from ring buffer without consuming yet */
            for (i_1 = 0U; i_1 < COM_RX_FRAME_SIZE; i_1++) 
            {
                rx_frame[i_1] = Uart_GetRxBufferByte(Uart_GetRxPeekIndex(i_1));
            }

            /* Calculate payload checksum over the first 10 bytes */
            uint8_t calc_checksum = 0U;

            for (i_2 = 0U;i_2 < 10U;i_2++)
            {
                calc_checksum += rx_frame[i_2];
            }

            /* Validate checksum against frame index 10 */
            if (calc_checksum == rx_frame[10])
            {
                /* Valid packet: Advance buffer tail by 12 positions */
                for (i_3 = 0U;i_3< COM_RX_FRAME_SIZE; i_3++) 
                {
                    (void)Uart_RxByte(); 
                }

                /* Pass validated frame to PDU Router */
                PduR_ComRxIndication(&rx_frame[0]);
            }
            else 
            {
                /* Invalid checksum: Drop 1 byte to attempt realignment */
                (void)Uart_RxByte();
            }
        }
        else 
        {
            /* Missing frame terminator (0x0D): Misaligned buffer. Pop 1 byte and search again */
            (void)Uart_RxByte();
        }
    }
} 


/**
 * Function name: Com_MainFunction_Tx
 * @brief Periodic cyclic transmission task (Designed to run every 10ms in OS scheduler).
 * Evaluates individual cyclic DID toggles and handles internal software prescalers.
 * @param: void
 * @return: void
 */
void Com_MainFunction_Tx(void)
{
    static uint16_t timer_adc_ms   = 0U;
    static uint16_t timer_pwm_ms   = 0U;
    static uint16_t timer_pid_ms   = 0U;
    static uint16_t timer_act_ms   = 0U;
    static uint16_t timer_fault_ms = 0U;

    /* Base task call interval: 10ms */
    timer_adc_ms   += 10U;
    timer_pwm_ms   += 10U;
    timer_pid_ms   += 10U;
    timer_act_ms   += 10U;
    timer_fault_ms += 10U;

    /* 1. Send ADC Monitoring Frame (0xAA) every 50ms if cyclic updates enabled via DID 0x0110 */
    if ((s_cyclic_conv_updates  != 0U) && (timer_adc_ms >= 50U))
    {
        timer_adc_ms = 0U;
        Com_Send_DiagFrame_ConvMonitorData();
    }

    /* 2. Send PWM Info Frame (0xA2) every 100ms if cyclic updates enabled via DID 0x0108 */
    if ((s_cyclic_pwm_updates != 0U) && (timer_pwm_ms >= 100U))
    {
        timer_pwm_ms = 0U;
        Com_Send_DiagFrame_PWMInfo();
    }

    /* 3. Send PID Info Frame (0xA3) every 100ms if cyclic updates enabled via DID 0x0107 */
    if ((s_cyclic_pid_updates != 0U) && (timer_pid_ms >= 100U))
    {
        timer_pid_ms = 0U;
        Com_Send_DiagFrame_PIDInfo();
    }

    /* 4. Send Actuator Status Frame (0xA4) every 200ms if cyclic updates enabled via DID 0x0109 */
    if ((s_cyclic_act_updates != 0U) && (timer_act_ms >= 200U))
    {
        timer_act_ms = 0U;
        Com_Send_DiagFrame_ActuatorInfo();
    }

    /* 5. Send Active DTC Memory Frame (0xA5) every 500ms if cyclic updates enabled via DID 0x0106 */
    if ((s_cyclic_fault_updates != 0U) && (timer_fault_ms >= 500U))
    {
        timer_fault_ms = 0U;
        Com_Send_ActiveDTC_Frames();
    }
}

/* ----------------------------- FRAME TX FUNCTIONS -------------------------------------------*/

/**
 * Function name: Com_Send_DiagFrame_ConvMonitorData
 * @brief Packets raw ADC channel readings into a diagnostic frame (Header 0xAA)
 * and transmits it over UART along with an alive counter and calculated checksum.
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_ConvMonitorData(void)
{
	uint8_t i;
    uint8_t checksum = 0U;
    
    /* 1. Header byte */
    Uart_TxByte(0xAAU);
    checksum += 0xAAU;
    
    /* 2. ADC Data payload (4 channels x 2 bytes = 8 bytes) */
	uint16_t raw_adc_buffer[ADC_NUM_CHANNELS];
	if (Adc_IsScanDone() == TRUE) 
    {
        /* Read ADC raw buffer data and populate local buffer */
        Adc_ReadGroup(raw_adc_buffer);
	}
	
	/* Send Adc Conv Data */
    for (i = 0U; i < ADC_NUM_CHANNELS; i++)
    {
        uint16_t val = raw_adc_buffer[i];
        uint8_t low_byte = (uint8_t)(val & 0xFFU);
        uint8_t high_byte = (uint8_t)((val >> 8U) & 0xFFU);
        
        Uart_TxByte(low_byte);  
        checksum += low_byte;
        Uart_TxByte(high_byte); 
        checksum += high_byte;
    }
    
    /* 3. Alive counter byte */
    Uart_TxByte(s_alive_counter_DiagFrame_ConvMonitorData); 
    checksum += s_alive_counter_DiagFrame_ConvMonitorData;

    /* 4. Checksum and frame terminator */
    Uart_TxByte(checksum);
    Uart_TxByte(0x0DU); 

    /* Increment rolling 4-bit alive counter (0 to 15) */
    s_alive_counter_DiagFrame_ConvMonitorData = (s_alive_counter_DiagFrame_ConvMonitorData + 1U) % 16U;
}

/**
 * Function name: Com_Send_DiagFrame_PWMInfo
 * @brief Transmits hardware PWM status, OCR register values, and frequency (Header 0xA2).
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_PWMInfo(void)
{
    uint8_t checksum = 0U;
    uint8_t bridge_status = 0U; 
    
    uint16_t ocr1a_val = Pwm_ReadOcr1A();
    uint16_t ocr1b_val = Pwm_ReadOcr1B();
    uint16_t top_val   = Pwm_ReadIcr1();

    /* Calculate current operational PWM frequency from timer registers */
    uint16_t pwm_frequency_hz = (uint16_t)(16000000UL / (8UL * (1UL + top_val))); 

    /* Determine bridge status based on OCR register limits */
    if (ocr1a_val >= (uint16_t)(top_val * 0.99f))       { bridge_status = 1U; } 
    else if ((ocr1a_val <= 10U) || (ocr1b_val <= 10U))  { bridge_status = 2U; } 
    else                                                { bridge_status = 0U; }

    /* 1. Header byte */
    Uart_TxByte(0xA2U); 
    checksum += 0xA2U;
    
    /* 2. OCR1A (2 bytes: Low byte first) */
    uint8_t ocr1a_low = (uint8_t)(ocr1a_val & 0xFFU);
    uint8_t ocr1a_high = (uint8_t)((ocr1a_val >> 8U) & 0xFFU);
    Uart_TxByte(ocr1a_low);  
    checksum += ocr1a_low;
    Uart_TxByte(ocr1a_high); 
    checksum += ocr1a_high;

    /* 3. OCR1B (2 bytes: Low byte first) */
    uint8_t ocr1b_low = (uint8_t)(ocr1b_val & 0xFFU);
    uint8_t ocr1b_high = (uint8_t)((ocr1b_val >> 8U) & 0xFFU);
    Uart_TxByte(ocr1b_low);  
    checksum += ocr1b_low;
    Uart_TxByte(ocr1b_high); 
    checksum += ocr1b_high;

    /* 4. Hardware status (1 byte) */
    Uart_TxByte(bridge_status); 
    checksum += bridge_status;

    /* 5. PWM Frequency (2 bytes: Low byte first) */
    uint8_t freq_low = (uint8_t)(pwm_frequency_hz & 0xFFU);
    uint8_t freq_high = (uint8_t)((pwm_frequency_hz >> 8U) & 0xFFU);
    Uart_TxByte(freq_low);  
    checksum += freq_low;
    Uart_TxByte(freq_high); 
    checksum += freq_high;

    /* 6. Padding byte */
    Uart_TxByte(0x00U); 
    checksum += 0x00U;
    
    /* 7. Alive counter, checksum, and terminator */
    Uart_TxByte(s_alive_counter_DiagFrame_PWMInfo); 
    checksum += s_alive_counter_DiagFrame_PWMInfo;
    
    Uart_TxByte(checksum);
    Uart_TxByte(0x0DU); 

    s_alive_counter_DiagFrame_PWMInfo = (s_alive_counter_DiagFrame_PWMInfo + 1U) % 16U;
}


/**
 * Function name: Com_Send_DiagFrame_PIDInfo
 * @brief Transmits PID controller parameters including target setpoint, output, and converter mode (Header 0xA3).
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_PIDInfo(void)
{
	uint8_t i;
    uint8_t checksum = 0U;
    
    /* 1. Header byte */
    Uart_TxByte(0xA3U); 
    checksum += 0xA3U;

	/* Fetch current values from RTE interfaces */
    uint16_t setpoint = Rte_Read_PidTargetSetpoint();
    uint8_t last_out = Rte_Read_LastPidOutput();
    uint8_t conv_state = Rte_Read_ConverterState();
    
    /* 2. Current Setpoint (2 bytes) */
    uint8_t sp_high = (uint8_t)((setpoint >> 8U) & 0xFFU);
    uint8_t sp_low  = (uint8_t)(setpoint & 0xFFU);
    Uart_TxByte(sp_low);   
    checksum += sp_low;
    Uart_TxByte(sp_high);  
    checksum += sp_high;

    /* 3. PID Output (2 bytes - scaled by 10 for decimal precision) */
    uint16_t pid_out_raw = (uint16_t)(last_out * 10.0f);
    uint8_t out_low = (uint8_t)(pid_out_raw & 0xFFU);
    uint8_t out_high = (uint8_t)((pid_out_raw >> 8U) & 0xFFU);
    Uart_TxByte(out_low);  
    checksum += out_low;
    Uart_TxByte(out_high); 
    checksum += out_high;

    /* 4. Converter state (1 byte) */
    Uart_TxByte(conv_state); 
    checksum += conv_state;

    /* 5. Padding (3 bytes) */
    for (i = 0U; i < 3U; i++) 
    {
        Uart_TxByte(0x00U);
        checksum += 0x00U;
    }
    
    /* 6. Alive counter, checksum, and terminator */
    Uart_TxByte(s_alive_counter_DiagFrame_PID); 
    checksum += s_alive_counter_DiagFrame_PID;
    
    Uart_TxByte(checksum);
    Uart_TxByte(0x0DU); 

    s_alive_counter_DiagFrame_PID = (s_alive_counter_DiagFrame_PID + 1U) % 16U;
}

/**
 * Function name: Com_Send_DiagFrame_ActuatorInfo
 * @brief Transmits digital relay status for input and output actuators (Header 0xA4).
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_ActuatorInfo(void)
{
	uint8_t i;
    uint8_t checksum = 0U;
    
    /* 1. Header byte */
    Uart_TxByte(0xA4U); 
    checksum += 0xA4U;

	/* Fetch relay states directly from DIO driver */
    uint8_t relay_in = (uint8_t)Dio_ReadChannel(DIO_CHANNEL_RELAY_IN);
    uint8_t relay_out = (uint8_t)Dio_ReadChannel(DIO_CHANNEL_RELAY_OUT);

    /* 2. Relay states */
    Uart_TxByte(relay_in);  
    checksum += relay_in;
    Uart_TxByte(relay_out); 
    checksum += relay_out;

    /* 3. Padding (6 bytes) */
    for (i = 0U; i < 6U; i++) 
    {
        Uart_TxByte(0x00U);
        checksum += 0x00U;
    }
    
    /* 4. Alive Counter, Checksum, and Terminator */
    Uart_TxByte(s_alive_counter_DiagFrame_Act); 
    checksum += s_alive_counter_DiagFrame_Act;
    
    Uart_TxByte(checksum);
    Uart_TxByte(0x0DU); 

    s_alive_counter_DiagFrame_Act = (s_alive_counter_DiagFrame_Act + 1U) % 16U;
}

/**
 * Function name: Com_Send_ActiveDTC_Frames
 * @brief Multi-frame transmission handler for active and passive Diagnostic Trouble Codes (Header 0xA5).
 * Uses dynamic DTC count query from DemSf.
 * @param: void
 * @return: void
 */
void Com_Send_ActiveDTC_Frames(void)
{
	uint8_t i;
	uint8_t f;
	uint8_t p;
	uint8_t d;
	int8_t b;
	uint8_t p_2;

    uint8_t total_dtcs = Dem_GetTotalDtcs();
    uint8_t active_indices[TOTAL_DTCS_MAX];
    uint8_t active_count = 0U;

    /* Filter active and passive faults from DEM table */
    for (i = 0U; i < total_dtcs; i++)
    {
        if (g_dtc_table[i].fault_status != DTC_STATUS_MISSING)
        {
            active_indices[active_count++] = i;
        }
    }

    /* Send empty frame if no fault is active (clears UI error log) */
    if (active_count == 0U)
    {
        uint8_t checksum = 0U;
        Uart_TxByte(0xA5U); 
        checksum += 0xA5U;
        
        for (p = 0U; p < 8U; p++)
        {
            Uart_TxByte(0x00U); 
            checksum += 0x00U;
        }

        Uart_TxByte(s_alive_counter_DiagFrame_DTC); 
        checksum += s_alive_counter_DiagFrame_DTC;
        
        Uart_TxByte(checksum);
        Uart_TxByte(0x0DU);

        s_alive_counter_DiagFrame_DTC = (s_alive_counter_DiagFrame_DTC + 1U) % 16U;
        return;
    }

    /* Calculate total required 12-byte frames (2 DTCs per frame) */
    uint8_t total_frames = (active_count + 1U) / 2U; 

    for (f = 0U; f < total_frames; f++)
    {
        uint8_t checksum = 0U;

        Uart_TxByte(0xA5U); 
        checksum += 0xA5U;

        for (d = 0U; d < 2U; d++)
        {
            uint8_t dtc_idx_array_pos = (f * 2U) + d;
            if (dtc_idx_array_pos < active_count)
            {
                uint8_t real_dtc_index = active_indices[dtc_idx_array_pos];
                uint32_t code = g_dtc_table[real_dtc_index].dtc_code;
                uint8_t status = g_dtc_table[real_dtc_index].fault_status;

                /* Pack 24-bit DTC code */
                for (b = 2; b >= 0; b--)
                {
                    uint8_t byte_val = (uint8_t)((code >> (b * 8)) & 0xFFU);
                    Uart_TxByte(byte_val); 
                    checksum += byte_val;
                }
                
                Uart_TxByte(status); 
                checksum += status;
            }
            else
            {
                for (p_2 = 0U; p_2< 4U; p_2++)
                {
                    Uart_TxByte(0x00U); 
                    checksum += 0x00U;
                }
            }
        }

        Uart_TxByte(s_alive_counter_DiagFrame_DTC); 
        checksum += s_alive_counter_DiagFrame_DTC;
        
        Uart_TxByte(checksum);
        Uart_TxByte(0x0DU);

        s_alive_counter_DiagFrame_DTC = (s_alive_counter_DiagFrame_DTC + 1U) % 16U;
    }
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of ComSf.c */