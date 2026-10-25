#ifndef SAFETY_EVENTS_H
#define SAFETY_EVENTS_H
/* Bits of Rte SafetyCauses (written by SWC_SafetyMonitor, read by the DEM monitor) */
#define SAFE_CAUSE_OVERCURRENT_IN   0x01U
#define SAFE_CAUSE_OVERCURRENT_OUT  0x02U
#define SAFE_CAUSE_OVERVOLT_IN      0x04U
#define SAFE_CAUSE_UNDERVOLT_IN     0x08U
#define SAFE_CAUSE_OVERVOLT_OUT     0x10U
#define SAFE_CAUSE_FUSE_BLOWN       0x20U
#define SAFE_CAUSE_OVERTEMP_CRIT    0x40U
#define SAFE_CAUSE_SENSOR_FAULT     0x80U

/* Rte FuseState values */
#define FUSE_STATE_OK       0U
#define FUSE_STATE_BLOWN    1U
#define FUSE_STATE_UNKNOWN  2U
#endif
