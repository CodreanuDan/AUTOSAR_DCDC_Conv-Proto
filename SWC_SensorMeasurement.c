/*
 * Filename: SWC_SensorMeasurement.c
 * @brief: Contains sensor reading, digital moving average filtering, and physical conversion algorithms for ASW
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "SWC_SensorMeasurement.h"
#include "Rte.h"

/*******************************************************
 *            STATIC LOCAL VARIABLES (ASW Cache)
 *******************************************************/
static float s_asw_physical_iin  = 0.0f;
static float s_asw_physical_vin  = 0.0f;
static float s_asw_physical_iout = 0.0f;
static float s_asw_physical_vout = 0.0f;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

void SWC_SensorMeasurement_Runnable(void)
{
    /* Read physical values mediated and filtered by BSW/IoHwAb into RTE */
    s_asw_physical_iin  = Rte_Read_Physical_Iin();
    s_asw_physical_vin  = Rte_Read_Physical_Vin();
    s_asw_physical_iout = Rte_Read_Physical_Iout();
    s_asw_physical_vout = Rte_Read_Physical_Vout();
}

float SWC_SensorMeasurement_GetVin(void)
{
    return s_asw_physical_vin;
}

float SWC_SensorMeasurement_GetVout(void)
{
    return s_asw_physical_vout;
}

float SWC_SensorMeasurement_GetIin(void)
{
    return s_asw_physical_iin;
}

float SWC_SensorMeasurement_GetIout(void)
{
    return s_asw_physical_iout;
}

/************* END OF FUNCTION DEFINITIONS ************/