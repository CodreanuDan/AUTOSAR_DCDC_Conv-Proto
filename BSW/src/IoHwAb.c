/*
 * Filename: IoHwAb.c
 * @brief: Implements calibrated physical conversions, multi-channel moving average filtering, and RTE routing
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "IoHwAb.h"
#include "Rte.h"
#include "PwmSf.h"
#include "DioSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *********************************--------------------**/
/* Dedicated filter instances for all four system analog channels */
static MovAvg_HandleType s_filt_iin;
static MovAvg_HandleType s_filt_vin;
static MovAvg_HandleType s_filt_iout;
static MovAvg_HandleType s_filt_vout;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

/*
 * Function name: MovAvg_Init
 * @brief Resets history buffer and operational variables for a specific moving average filter instance.
 * @param: MovAvg_HandleType *f (Pointer to filter instance)
 * @return: void
 */
void MovAvg_Init(MovAvg_HandleType *f) 
{
    uint8_t i;
    for (i = 0U; i < FILTER_WINDOW_SIZE; i++) 
    {
        f->buffer[i] = 0.0f;
    }
    f->idx = 0U;
    f->filled = 0U;
    f->sum = 0.0f;
}

/*
 * Function name: Filters_Init
 * @brief Initializes moving average filter instances for all monitored signals.
 * @param: void
 * @return: void
 */
void Filters_Init(void) 
{
    MovAvg_Init(&s_filt_iin);
    MovAvg_Init(&s_filt_vin);
    MovAvg_Init(&s_filt_iout);
    MovAvg_Init(&s_filt_vout);
}

/*
 * Function name: MovAvg_Update
 * @brief Pushes a new raw sample into the sliding window and computes the smoothed arithmetic mean.
 * @param: MovAvg_HandleType *f, float new_sample
 * @return: float smoothed output value
 */
float MovAvg_Update(MovAvg_HandleType *f, float new_sample) 
{
    f->sum -= f->buffer[f->idx];
    f->buffer[f->idx] = new_sample;
    f->sum += new_sample;
    f->idx = (f->idx + 1U) % FILTER_WINDOW_SIZE;
    if (f->filled < FILTER_WINDOW_SIZE) 
    {
        f->filled++;
    }
    return f->sum / (float)f->filled;
}

/*
 * Function name: IoHwAb_Analog_ConvertToAmps
 * @brief Converts raw 10-bit counts to Amperes using ACS712 sensor offsets and sensitivity calibration constants.
 * @param: uint16_t raw
 * @return: float calculated current in Amperes
 */
float IoHwAb_Analog_ConvertToAmps(uint16_t raw)
{
    /* Step 1: Translate digital 10-bit count into voltage representation (0V - 5V) */
    float voltage = ((float)raw / ADC_RESOLUTION_MAX) * ADC_REF_VOLTAGE;
    
    /* Step 2: Subtract quiescent zero-current output voltage (2.5V) and scale by sensor sensitivity */
    return (voltage - ACS712_ZERO_OFFSET) / ACS712_SENSITIVITY;
}

/*
 * Function name: IoHwAb_Analog_ReadVoltage
 * @brief Converts raw 10-bit counts to Volts using the hardware resistor divider scaling factor.
 * @param: uint16_t raw
 * @return: float calculated voltage in Volts
 */
float IoHwAb_Analog_ReadVoltage(uint16_t raw)
{
    /* Step 1: Calculate measured voltage across the MCU pin (0V - 5V range) */
    float adc_pin_voltage = ((float)raw / ADC_RESOLUTION_MAX) * ADC_REF_VOLTAGE;
    
    /* Step 2: Apply the external voltage divider scaling multiplier */
    return adc_pin_voltage * VDIV_SCALING_FACTOR;
}

/*
 * Function name: IoHwAb_Sensor_MainFunction
 * @brief Periodically processes raw ADC scans, executes noise filtering on currents and voltages, and updates RTE.
 * @param: void
 * @return: void
 */
void IoHwAb_Sensor_MainFunction(void)
{
    if (Rte_Read_AdcScanDone() == TRUE) 
    {
        /* 1. Process Input Current (Iin): Convert to physical units and filter transient noise */
        uint16_t raw_iin = Rte_Read_AdcRaw_Iin();
        float physical_iin = IoHwAb_Analog_ConvertToAmps(raw_iin);
        float filtered_iin = MovAvg_Update(&s_filt_iin, physical_iin);
        Rte_Write_Physical_Iin(filtered_iin);

        /* 2. Process Input Voltage (Vin): Convert via resistor divider rules and filter ripples */
        uint16_t raw_vin = Rte_Read_AdcRaw_Vin();
        float physical_vin = IoHwAb_Analog_ReadVoltage(raw_vin);
        float filtered_vin = MovAvg_Update(&s_filt_vin, physical_vin);
        Rte_Write_Physical_Vin(filtered_vin);

        /* 3. Process Output Current (Iout): Convert to physical units and filter transient noise */
        uint16_t raw_iout = Rte_Read_AdcRaw_Iout();
        float physical_iout = IoHwAb_Analog_ConvertToAmps(raw_iout);
        float filtered_iout = MovAvg_Update(&s_filt_iout, physical_iout);
        Rte_Write_Physical_Iout(filtered_iout);

        /* 4. Process Output Voltage (Vout): Convert via resistor divider rules and filter ripples */
        uint16_t raw_vout = Rte_Read_AdcRaw_Vout();
        float physical_vout = IoHwAb_Analog_ReadVoltage(raw_vout);
        float filtered_vout = MovAvg_Update(&s_filt_vout, physical_vout);
        Rte_Write_Physical_Vout(filtered_vout);
    }
}

/*
 * Function name: IoHwAb_Actuator_MainFunction
 * @brief Translates RTE logical actuator requests into direct MCAL hardware executions.
 * @param: void
 * @return: void
 */
void IoHwAb_Actuator_MainFunction(void)
{
    uint8_t duty_a = Rte_Read_DutyA();
    uint8_t duty_b = Rte_Read_DutyB();
    Pwm_SetDutyCycle(duty_a, duty_b);

    uint8_t relay_in_cmd = Rte_Read_RelayInputState();
    uint8_t relay_out_cmd = Rte_Read_RelayOutputState();
    Dio_WriteChannel(DIO_CHANNEL_RELAY_IN, (relay_in_cmd > 0U) ? STD_HIGH : STD_LOW);
    Dio_WriteChannel(DIO_CHANNEL_RELAY_OUT, (relay_out_cmd > 0U) ? STD_HIGH : STD_LOW);
}
/************* END OF FUNCTION DEFINITIONS ************/