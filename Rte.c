/**
 * @file Rte.c
 * @brief Auto-generated RTE Source Implementation for DC_DC_Analyzer_Pro
 * @date Generated automatically by Mini-Tresos RTE Generator Tool
 */

#include "Rte.h"

/* =====================================================================
 * RTE INTERNAL STORAGE (Encapsulated Global Variables)
 * ===================================================================== */
static uint8_t RTE_VAR_DutyA = 0U;
static uint8_t RTE_VAR_DutyB = 0U;
static uint16_t RTE_VAR_TargetFrequency = 0U;
static float RTE_VAR_Physical_Iin = 0.0f;
static float RTE_VAR_Physical_Vin = 0.0f;
static float RTE_VAR_Physical_Iout = 0.0f;
static float RTE_VAR_Physical_Vout = 0.0f;
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
static uint8_t RTE_VAR_TickMs = 0U;
static uint8_t RTE_VAR_Kl15State = 0U;
static uint8_t RTE_VAR_CoolantLevel      = 0U;
static uint8_t RTE_VAR_CoolantLevelRaw   = 0U;
static uint8_t RTE_VAR_LevelSensorValid  = 0U;
static uint8_t RTE_VAR_FanDuty = 0;  
static int16_t  RTE_VAR_AmbTemp = 0;
static uint16_t RTE_VAR_AmbHumidity = 0;
static uint8_t RTE_VAR_Sht21Valid = 0;
static uint8_t RTE_VAR_FanMode = 0;
static uint8_t RTE_VAR_FanManualDuty = 0;
static uint8_t RTE_VAR_CoolState = 0;
static uint8_t RTE_VAR_CoolFaults = 0;
static uint8_t RTE_VAR_FuseSense = 1; /* debounced PB5 level (1 = high, 0 = low) */
static uint8_t RTE_VAR_FuseState = 2; /* 0 OK, 1 BLOWN, 2 UNKNOWN*/
static uint8_t RTE_VAR_SafetyCauses = 0;

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

void Rte_Write_Physical_Iin(float val) 
{
    RTE_VAR_Physical_Iin = val;
}

float Rte_Read_Physical_Iin(void) 
{
    return RTE_VAR_Physical_Iin;
}

void Rte_Write_Physical_Vin(float val) 
{
    RTE_VAR_Physical_Vin = val;
}

float Rte_Read_Physical_Vin(void) 
{
    return RTE_VAR_Physical_Vin;
}

void Rte_Write_Physical_Iout(float val) 
{
    RTE_VAR_Physical_Iout = val;
}

float Rte_Read_Physical_Iout(void) 
{
    return RTE_VAR_Physical_Iout;
}

void Rte_Write_Physical_Vout(float val) 
{
    RTE_VAR_Physical_Vout = val;
}

float Rte_Read_Physical_Vout(void) 
{
    return RTE_VAR_Physical_Vout;
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

void Rte_Write_TickMs(uint8_t val) 
{
    RTE_VAR_TickMs = val;
}

uint8_t Rte_Read_TickMs(void) 
{
    return RTE_VAR_TickMs;
}

void Rte_Write_Kl15State(uint8_t val) 
{ 
	RTE_VAR_Kl15State = val; 
}

uint8_t Rte_Read_Kl15State(void) 
{ 
	return RTE_VAR_Kl15State; 
}

void Rte_Write_CoolantLevel(uint8_t val)
{
	RTE_VAR_CoolantLevel = val;
} 

uint8_t Rte_Read_CoolantLevel(void)
{
	return RTE_VAR_CoolantLevel;
}

void Rte_Write_CoolantLevelRaw(uint8_t val)
{
	 RTE_VAR_CoolantLevelRaw = val;
}

uint8_t Rte_Read_CoolantLevelRaw(void)
{
	return RTE_VAR_CoolantLevelRaw;
}

void Rte_Write_LevelSensorValid(uint8_t val)
{
	RTE_VAR_LevelSensorValid = val;
}  

uint8_t Rte_Read_LevelSensorValid(void)
{
	return RTE_VAR_LevelSensorValid;
}

void Rte_Write_FanDuty(uint8_t val)
{
	RTE_VAR_FanDuty = val;
}

uint8_t Rte_Read_FanDuty(void)
{
	return RTE_VAR_FanDuty;
}

void Rte_Write_AmbTemp(int16_t val)
{
	RTE_VAR_AmbTemp = val;
}

int16_t Rte_Read_AmbTemp(void)
{
	return RTE_VAR_AmbTemp;
}

void Rte_Write_AmbHumidity(uint16_t val)
{
	RTE_VAR_AmbHumidity = val;
}

uint16_t Rte_Read_AmbHumidity(void)
{
	return RTE_VAR_AmbHumidity;
}

void Rte_Write_Sht21Valid(uint8_t val)
{
	RTE_VAR_Sht21Valid = val;
}

uint8_t Rte_Read_Sht21Valid(void)
{
	return RTE_VAR_Sht21Valid;
}

void Rte_Write_FanMode(uint8_t val) 
{
	RTE_VAR_FanMode = val;
}

uint8_t Rte_Read_FanMode(void)
{
	return RTE_VAR_FanMode;
}

void Rte_Write_FanManualDuty(uint8_t val)
{
	RTE_VAR_FanManualDuty = val;
}

uint8_t Rte_Read_FanManualDuty(void)
{
	return RTE_VAR_FanManualDuty;
}

void Rte_Write_CoolState(uint8_t val) 
{
	RTE_VAR_CoolState = val;
}
   
uint8_t Rte_Read_CoolState(void)
{
	return RTE_VAR_CoolState;
}

void Rte_Write_CoolFaults(uint8_t val)
{
	RTE_VAR_CoolFaults = val;
}

uint8_t Rte_Read_CoolFaults(void)
{
	return RTE_VAR_CoolFaults;
}

void Rte_Write_FuseState(uint8_t val)
{
	RTE_VAR_FuseState = val;
}

uint8_t Rte_Read_FuseState(void)
{
	return RTE_VAR_FuseState;
}

void Rte_Write_FuseSense(uint8_t val)
{
	RTE_VAR_FuseSense = val;
}

uint8_t Rte_Read_FuseSense(void)
{
	return RTE_VAR_FuseSense;
}

void Rte_Write_SafetyCauses(uint8_t val)
{
	RTE_VAR_SafetyCauses = val;
}

uint8_t Rte_Read_SafetyCauses(void)
{
	return RTE_VAR_SafetyCauses;
}



