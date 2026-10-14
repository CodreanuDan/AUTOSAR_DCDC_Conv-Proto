/* SWC_CoolingCtrl.h */
#ifndef SWC_COOLINGCTRL_H
#define SWC_COOLINGCTRL_H

#include "StdTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Called every 10 ms from the task in RUN and PRE_SLEEP.
 * afterrun_active = 1 while EcuM is in PRE_SLEEP.
 * This is the ONLY writer of Rte FanDuty / CoolState (except the EcuM shutdown clear).
 */
void SWC_CoolingCtrl_Runnable(uint8_t afterrun_active);

#ifdef __cplusplus
}
#endif

#endif /* SWC_COOLINGCTRL_H */

