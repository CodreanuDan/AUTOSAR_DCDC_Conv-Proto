#ifndef SAFETY_EVENTS_H
#define SAFETY_EVENTS_H

/* Bits of Rte SafetyCauses (written by SWC_SafetyMonitor, read by the DEM monitor)
 * Bits of Rte SafetyCauses = currently ACTIVE (debounced) causes  
 */

#define SAFE_CAUSE_OVERCURRENT_IN    0x01U   /* Iin  > 8 A            */
#define SAFE_CAUSE_OVERCURRENT_OUT   0x02U   /* Iout > 8 A            */
#define SAFE_CAUSE_OVERVOLT_IN       0x04U   /* Vin  > 16 V           */
#define SAFE_CAUSE_UNDERVOLT_IN      0x08U   /* Vin  < 9 V            */
#define SAFE_CAUSE_OVERVOLT_OUT      0x10U   /* Vout > max(sp*1.2, sp+3) */
#define SAFE_CAUSE_FUSE_BLOWN        0x20U
#define SAFE_CAUSE_OVERTEMP_CRIT     0x40U   /* T >= 100 degC         */
#define SAFE_CAUSE_OVERVOLT_IN_CRIT  0x80U   /* Vin  > 24 V           */

/* REACTION TABLE: causes in this mask stop the converter. Everything else = DTC only.
 * To change a reaction, edit only this line. */
#define SAFE_SHUTDOWN_MASK  (SAFE_CAUSE_OVERCURRENT_IN | SAFE_CAUSE_OVERTEMP_CRIT | SAFE_CAUSE_OVERVOLT_IN_CRIT)

/* Rte FuseState values */
#define FUSE_STATE_OK       0U
#define FUSE_STATE_BLOWN    1U
#define FUSE_STATE_UNKNOWN  2U

/* Rte SafetyState (read-only status) */
#define SAFE_STATE_OK        0U
#define SAFE_STATE_DEGRADED  1U   /* DTC-only cause active, converter running */
#define SAFE_STATE_SHUTDOWN  2U

/* Rte RecoveryMode */
#define SAFE_RECOVERY_MANUAL 0U
#define SAFE_RECOVERY_AUTO   1U

#endif

