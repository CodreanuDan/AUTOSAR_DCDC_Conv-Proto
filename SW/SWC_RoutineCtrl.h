/*
 * Filename: SWC_RoutineCtrl.h
 * @brief: UDS 0x31 diagnostic test routines (PWM sweeps, PID setpoint steps, relay toggling)
 */
#ifndef SWC_ROUTINECTRL_H
#define SWC_ROUTINECTRL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Routine identifiers (RID) */
#define ROUTINE_RID_PWM_BOOST     0x0201U   /* duty A fixed, duty B swept 90 % -> 10 %    (PID must be DISABLED) */
#define ROUTINE_RID_PWM_BUCK      0x0202U   /* duty A swept 90 % -> 10 %, duty B fixed    (PID must be DISABLED) */
#define ROUTINE_RID_PID_BOOST     0x0203U   /* setpoint = Vin + 2, 4 ... 18 V, then Vin   (PID must be ENABLED)  */
#define ROUTINE_RID_PID_BUCK      0x0204U   /* setpoint = Vin - 1, 2 ... 9 V, then Vin    (PID must be ENABLED)  */
#define ROUTINE_RID_RELAY_IN      0x0205U   /* input relay toggled 10 x ON/OFF            (PID must be DISABLED) */
#define ROUTINE_RID_RELAY_OUT     0x0206U   /* output relay toggled 10 x ON/OFF           (PID must be DISABLED) */

/* Routine status (record byte of the 0x31 response, also returned by sub-function 0x03) */
#define ROUTINE_STATUS_NOT_RUN    0U
#define ROUTINE_STATUS_RUNNING    1U
#define ROUTINE_STATUS_COMPLETED  2U
#define ROUTINE_STATUS_STOPPED    3U        /* stopped by the tester (31 02)                         */
#define ROUTINE_STATUS_ABORTED    4U        /* safety shutdown, ECU left RUN, PID mode changed, session left */

/** Returns TRUE if the RID belongs to one of the routines above. */
bool SWC_RoutineCtrl_IsKnown(uint16_t rid);

/** 31 01: returns 0 when started, otherwise the NRC (0x31 unknown RID, 0x22 conditions not correct). */
uint8_t SWC_RoutineCtrl_Start(uint16_t rid);

/** 31 02: stops the routine if it is running (always positive for a known RID). Returns 0 or NRC 0x31. */
uint8_t SWC_RoutineCtrl_Stop(uint16_t rid);

/** 31 03: status and current step (number of finished steps when the routine is over). Returns 0 or NRC 0x31. */
uint8_t SWC_RoutineCtrl_GetResult(uint16_t rid, uint8_t *status, uint8_t *step);

/** Aborts the running routine (called when the diagnostic session returns to default). */
void SWC_RoutineCtrl_Abort(void);

/** Called every 100 ms. ecum_run: TRUE while the ECU is in RUN. One routine step lasts 500 ms. */
void SWC_RoutineCtrl_Runnable(bool ecum_run);

#ifdef __cplusplus
}
#endif

#endif /* SWC_ROUTINECTRL_H */
