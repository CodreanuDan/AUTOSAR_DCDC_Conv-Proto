/**
 * @file Rte.c
 * @brief Auto-generated RTE Source Implementation for DC_DC_Analyzer_Pro
 * @date Generated automatically by Mini-Tresos RTE Generator Tool
 */

#include "Rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * RTE INTERNAL STORAGE (Encapsulated Global Variables)
 * ===================================================================== */
static uint8_t RTE_VAR_DutyA = 0U;
static uint8_t RTE_VAR_DutyB = 0U;
static uint16_t RTE_VAR_TargetFrequency = 0U;
static uint16_t RTE_VAR_AdcRaw_Iin = 0U;
static uint16_t RTE_VAR_AdcRaw_Vin = 0U;
static uint16_t RTE_VAR_AdcRaw_Iout = 0U;
static uint16_t RTE_VAR_AdcRaw_Vout = 0U;
static uint8_t RTE_VAR_AdcScanDone = 0U;
static uint16_t RTE_VAR_PidTargetSetpoint = 0U;
static bool RTE_VAR_PidDisableFlag = false;
static uint8_t RTE_VAR_ConverterState = 0U;
static uint8_t RTE_VAR_LastPidOutput = 0U;
static uint8_t RTE_VAR_RelayInputState = 0U;
static uint8_t RTE_VAR_RelayOutputState = 0U;
static uint8_t RTE_VAR_RoutinePwmBoost = 0U;
static uint8_t RTE_VAR_RoutinePwmBuck = 0U;
static uint8_t RTE_VAR_RoutinePidBoost = 0U;
static uint8_t RTE_VAR_RoutinePidBuck = 0U;
static uint8_t RTE_VAR_RoutineInputRelay = 0U;
static uint8_t RTE_VAR_RoutineOutputRelay = 0U;
static uint8_t RTE_VAR_CyclicConvUpdates = 0U;
static uint8_t RTE_VAR_CyclicFaultUpdates = 0U;
static uint8_t RTE_VAR_CyclicPwmUpdates = 0U;
static uint8_t RTE_VAR_CyclicActUpdates = 0U;
static uint8_t RTE_VAR_CyclicPidUpdates = 0U;
static uint32_t RTE_VAR_TickMs = 0U;

/* =====================================================================
 * RTE API IMPLEMENTATIONS (Read & Write Functions)
 * ===================================================================== */
void Rte_Write_DutyA(uint8_t val) 
{
    RTE_VAR_DutyA = val;
}

uint8_t Rte_Read_DutyA(void) 
{
    return RTE_VAR_DutyA;
}

void Rte_Write_DutyB(uint8_t val) 
{
    RTE_VAR_DutyB = val;
}

uint8_t Rte_Read_DutyB(void) 
{
    return RTE_VAR_DutyB;
}

void Rte_Write_TargetFrequency(uint16_t val) 
{
    RTE_VAR_TargetFrequency = val;
}

uint16_t Rte_Read_TargetFrequency(void) 
{
    return RTE_VAR_TargetFrequency;
}

void Rte_Write_AdcRaw_Iin(uint16_t val) 
{
    RTE_VAR_AdcRaw_Iin = val;
}

uint16_t Rte_Read_AdcRaw_Iin(void) 
{
    return RTE_VAR_AdcRaw_Iin;
}

void Rte_Write_AdcRaw_Vin(uint16_t val) 
{
    RTE_VAR_AdcRaw_Vin = val;
}

uint16_t Rte_Read_AdcRaw_Vin(void) 
{
    return RTE_VAR_AdcRaw_Vin;
}

void Rte_Write_AdcRaw_Iout(uint16_t val) 
{
    RTE_VAR_AdcRaw_Iout = val;
}

uint16_t Rte_Read_AdcRaw_Iout(void) 
{
    return RTE_VAR_AdcRaw_Iout;
}

void Rte_Write_AdcRaw_Vout(uint16_t val) 
{
    RTE_VAR_AdcRaw_Vout = val;
}

uint16_t Rte_Read_AdcRaw_Vout(void) 
{
    return RTE_VAR_AdcRaw_Vout;
}

void Rte_Write_AdcScanDone(uint8_t val) 
{
    RTE_VAR_AdcScanDone = val;
}

uint8_t Rte_Read_AdcScanDone(void) 
{
    return RTE_VAR_AdcScanDone;
}

void Rte_Write_PidTargetSetpoint(uint16_t val) 
{
    RTE_VAR_PidTargetSetpoint = val;
}

uint16_t Rte_Read_PidTargetSetpoint(void) 
{
    return RTE_VAR_PidTargetSetpoint;
}

void Rte_Write_PidDisableFlag(bool val) 
{
    RTE_VAR_PidDisableFlag = val;
}

bool Rte_Read_PidDisableFlag(void) 
{
    return RTE_VAR_PidDisableFlag;
}

void Rte_Write_ConverterState(uint8_t val) 
{
    RTE_VAR_ConverterState = val;
}

uint8_t Rte_Read_ConverterState(void) 
{
    return RTE_VAR_ConverterState;
}

void Rte_Write_LastPidOutput(uint8_t val) 
{
    RTE_VAR_LastPidOutput = val;
}

uint8_t Rte_Read_LastPidOutput(void) 
{
    return RTE_VAR_LastPidOutput;
}

void Rte_Write_RelayInputState(uint8_t val) 
{
    RTE_VAR_RelayInputState = val;
}

uint8_t Rte_Read_RelayInputState(void) 
{
    return RTE_VAR_RelayInputState;
}

void Rte_Write_RelayOutputState(uint8_t val) 
{
    RTE_VAR_RelayOutputState = val;
}

uint8_t Rte_Read_RelayOutputState(void) 
{
    return RTE_VAR_RelayOutputState;
}

void Rte_Write_RoutinePwmBoost(uint8_t val) 
{
    RTE_VAR_RoutinePwmBoost = val;
}

uint8_t Rte_Read_RoutinePwmBoost(void) 
{
    return RTE_VAR_RoutinePwmBoost;
}

void Rte_Write_RoutinePwmBuck(uint8_t val) 
{
    RTE_VAR_RoutinePwmBuck = val;
}

uint8_t Rte_Read_RoutinePwmBuck(void) 
{
    return RTE_VAR_RoutinePwmBuck;
}

void Rte_Write_RoutinePidBoost(uint8_t val) 
{
    RTE_VAR_RoutinePidBoost = val;
}

uint8_t Rte_Read_RoutinePidBoost(void) 
{
    return RTE_VAR_RoutinePidBoost;
}

void Rte_Write_RoutinePidBuck(uint8_t val) 
{
    RTE_VAR_RoutinePidBuck = val;
}

uint8_t Rte_Read_RoutinePidBuck(void) 
{
    return RTE_VAR_RoutinePidBuck;
}

void Rte_Write_RoutineInputRelay(uint8_t val) 
{
    RTE_VAR_RoutineInputRelay = val;
}

uint8_t Rte_Read_RoutineInputRelay(void) 
{
    return RTE_VAR_RoutineInputRelay;
}

void Rte_Write_RoutineOutputRelay(uint8_t val) 
{
    RTE_VAR_RoutineOutputRelay = val;
}

uint8_t Rte_Read_RoutineOutputRelay(void) 
{
    return RTE_VAR_RoutineOutputRelay;
}

void Rte_Write_CyclicConvUpdates(uint8_t val) 
{
    RTE_VAR_CyclicConvUpdates = val;
}

uint8_t Rte_Read_CyclicConvUpdates(void) 
{
    return RTE_VAR_CyclicConvUpdates;
}

void Rte_Write_CyclicFaultUpdates(uint8_t val) 
{
    RTE_VAR_CyclicFaultUpdates = val;
}

uint8_t Rte_Read_CyclicFaultUpdates(void) 
{
    return RTE_VAR_CyclicFaultUpdates;
}

void Rte_Write_CyclicPwmUpdates(uint8_t val) 
{
    RTE_VAR_CyclicPwmUpdates = val;
}

uint8_t Rte_Read_CyclicPwmUpdates(void) 
{
    return RTE_VAR_CyclicPwmUpdates;
}

void Rte_Write_CyclicActUpdates(uint8_t val) 
{
    RTE_VAR_CyclicActUpdates = val;
}

uint8_t Rte_Read_CyclicActUpdates(void) 
{
    return RTE_VAR_CyclicActUpdates;
}

void Rte_Write_CyclicPidUpdates(uint8_t val) 
{
    RTE_VAR_CyclicPidUpdates = val;
}

uint8_t Rte_Read_CyclicPidUpdates(void) 
{
    return RTE_VAR_CyclicPidUpdates;
}

void Rte_Write_TickMs(uint32_t val) 
{
    RTE_VAR_TickMs = val;
}

uint32_t Rte_Read_TickMs(void) 
{
    return RTE_VAR_TickMs;
}


#ifdef __cplusplus
}
#endif