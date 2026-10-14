/* SWC_CoolingCtrl.c */
#include "SWC_CoolingCtrl.h"
#include "Rte.h"

/*******************************************************
 *            STATIC LOCAL VARIABLES (ASW Cache)
 *******************************************************/
#define FAN_MAX_DUTY        90U   /* never above 90 % (also the manual limit) */
#define FAN_AFTERRUN_DUTY   50U   /* PRE_SLEEP after-run, ignores auto and manual */

#define COOL_OFF 0U
#define COOL_RUN 1U
#define COOL_AFTERRUN 2U
#define COOL_FAILSAFE 3U

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/
void SWC_CoolingCtrl_Runnable(uint8_t afterrun_active)
{
    uint8_t duty;
    uint8_t state;

    if (afterrun_active != 0U)                 /* 1. after-run overrides everything */
    {
        duty  = FAN_AFTERRUN_DUTY;
        state = COOL_AFTERRUN;
    }
    else if (Rte_Read_FanMode() == 1U)         /* 2. manual from DID 0x0129 */
    {
        duty = Rte_Read_FanManualDuty();
        if (duty > FAN_MAX_DUTY) { duty = FAN_MAX_DUTY; }
        state = (duty > 0U) ? COOL_RUN : COOL_OFF;
    }
    else                                       /* 3. auto: curve goes here in the next step */
    {
        duty  = 0U;
        state = COOL_OFF;
    }

    Rte_Write_FanDuty(duty);
    Rte_Write_CoolState(state);
}

/************* END OF FUNCTION DEFINITIONS ************/