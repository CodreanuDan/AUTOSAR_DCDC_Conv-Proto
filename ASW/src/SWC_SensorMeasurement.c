/*
 * Filename: SWC_SensorMeasurement.c
 * @brief: Contains sensor reading, digital moving average filtering, and physical conversion algorithms for ASW
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "SWC_SensorMeasurement.h"
#include "Rte.h"
#include "StdTypes.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/
static float V_in = 0.0f;
static float V_out = 0.0f;

/* External Moving Average Filter instances and driver helper declarations from BSW/IoHwAb */
extern MovAvg_HandleType s_filt_iin;
extern MovAvg_HandleType s_filt_iout;

extern float IoHwAb_Analog_ConvertToAmps(uint16_t raw_val);
extern float IoHwAb_Analog_ReadVoltage(uint16_t raw_val);

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

/*
 * Function name: SWC_SensorMeasurement_Runnable
 * @brief Periodically reads raw ADC channels through RTE, applies filtering and physical conversion helpers.
 * @param: void
 * @return: void
 */
void SWC_SensorMeasurement_Runnable(void)
{
    /* Check if multi-channel ADC scan cycle is completed via RTE */
    if (Rte_Read_AdcScanDone() == TRUE) 
    {
        /* 1. Read and process Input Current channel (Iin) using conversion helper and Moving Average */
        uint16_t raw_iin = Rte_Read_AdcRaw_Iin();
        float raw_in_signed = IoHwAb_Analog_ConvertToAmps(raw_iin);    
        (void)MovAvg_Update(&s_filt_iin, raw_in_signed);

        /* 2. Read and process Input Voltage channel (Vin) using voltage divider conversion */
        uint16_t raw_vin = Rte_Read_AdcRaw_Vin();
        V_in = IoHwAb_Analog_ReadVoltage(raw_vin);    

        /* 3. Read and process Output Current channel (Iout) */
        uint16_t raw_iout = Rte_Read_AdcRaw_Iout();
        float raw_out_signed = IoHwAb_Analog_ConvertToAmps(raw_iout);    
        (void)MovAvg_Update(&s_filt_iout, raw_out_signed);

        /* 4. Read and process Output Voltage channel (Vout) */
        uint16_t raw_vout = Rte_Read_AdcRaw_Vout();
        V_out = IoHwAb_Analog_ReadVoltage(raw_vout);    
    }
}

/*
 * Function name: SWC_SensorMeasurement_GetVin
 * @brief Returns the filtered and converted input voltage value.
 * @param: void
 * @return: float V_in
 */
float SWC_SensorMeasurement_GetVin(void)
{
    return V_in;
}

/*
 * Function name: SWC_SensorMeasurement_GetVout
 * @brief Returns the filtered and converted output voltage value.
 * @param: void
 * @return: float V_out
 */
float SWC_SensorMeasurement_GetVout(void)
{
    return V_out;
}
/************* END OF FUNCTION DEFINITIONS ************/