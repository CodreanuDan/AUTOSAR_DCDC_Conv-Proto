/* SWC_SafetyMonitor.h */
#ifndef SWC_SAFETYMONITOR_H
#define SWC_SAFETYMONITOR_H

#include "StdTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Called every 10 ms in RUN, after SWC_SensorMeasurement_Runnable.
 * v_in: input voltage measured BEFORE the fuse (V).
 */
void SWC_SafetyMonitor_Runnable(float v_in);

#ifdef __cplusplus
}
#endif

#endif /* SWC_SAFETYMONITOR_H */