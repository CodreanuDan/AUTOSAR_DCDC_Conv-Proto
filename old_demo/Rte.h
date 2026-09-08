/**
 * @file Rte.h
 * @brief Auto-generated RTE Header for DC_DC_Analyzer_Pro
 */

#ifndef RTE_H
#define RTE_H

#include <stdint.h>
#include <stdbool.h>

#ifndef TRUE
#define TRUE  1U
#endif

#ifndef FALSE
#define FALSE 0U
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =====================================================================
 * RTE PORT-BASED API PROTOTYPES
 * ===================================================================== */
void Rte_Write_DutyA(uint8_t val);
uint8_t Rte_Read_DutyA(void);
void Rte_Write_DutyB(uint8_t val);
uint8_t Rte_Read_DutyB(void);
void Rte_Write_TargetFrequency(uint16_t val);
uint16_t Rte_Read_TargetFrequency(void);
void Rte_Write_AdcRaw_Iin(uint16_t val);
uint16_t Rte_Read_AdcRaw_Iin(void);
void Rte_Write_AdcRaw_Vin(uint16_t val);
uint16_t Rte_Read_AdcRaw_Vin(void);
void Rte_Write_AdcRaw_Iout(uint16_t val);
uint16_t Rte_Read_AdcRaw_Iout(void);
void Rte_Write_AdcRaw_Vout(uint16_t val);
uint16_t Rte_Read_AdcRaw_Vout(void);
void Rte_Write_AdcScanDone(uint8_t val);
uint8_t Rte_Read_AdcScanDone(void);
void Rte_Write_PidTargetSetpoint(uint16_t val);
uint16_t Rte_Read_PidTargetSetpoint(void);
void Rte_Write_PidDisableFlag(bool val);
bool Rte_Read_PidDisableFlag(void);
void Rte_Write_ConverterState(uint8_t val);
uint8_t Rte_Read_ConverterState(void);
void Rte_Write_LastPidOutput(uint8_t val);
uint8_t Rte_Read_LastPidOutput(void);
void Rte_Write_RelayInputState(uint8_t val);
uint8_t Rte_Read_RelayInputState(void);
void Rte_Write_RelayOutputState(uint8_t val);
uint8_t Rte_Read_RelayOutputState(void);
void Rte_Write_RoutinePwmBoost(uint8_t val);
uint8_t Rte_Read_RoutinePwmBoost(void);
void Rte_Write_RoutinePwmBuck(uint8_t val);
uint8_t Rte_Read_RoutinePwmBuck(void);
void Rte_Write_RoutinePidBoost(uint8_t val);
uint8_t Rte_Read_RoutinePidBoost(void);
void Rte_Write_RoutinePidBuck(uint8_t val);
uint8_t Rte_Read_RoutinePidBuck(void);
void Rte_Write_RoutineInputRelay(uint8_t val);
uint8_t Rte_Read_RoutineInputRelay(void);
void Rte_Write_RoutineOutputRelay(uint8_t val);
uint8_t Rte_Read_RoutineOutputRelay(void);
void Rte_Write_CyclicConvUpdates(uint8_t val);
uint8_t Rte_Read_CyclicConvUpdates(void);
void Rte_Write_CyclicFaultUpdates(uint8_t val);
uint8_t Rte_Read_CyclicFaultUpdates(void);
void Rte_Write_CyclicPwmUpdates(uint8_t val);
uint8_t Rte_Read_CyclicPwmUpdates(void);
void Rte_Write_CyclicActUpdates(uint8_t val);
uint8_t Rte_Read_CyclicActUpdates(void);
void Rte_Write_CyclicPidUpdates(uint8_t val);
uint8_t Rte_Read_CyclicPidUpdates(void);
void Rte_Write_TickMs(uint32_t val); // Înlocuie?te uint8_t cu uint32_t
uint32_t Rte_Read_TickMs(void);

#ifdef __cplusplus
}
#endif

#endif /* RTE_H */