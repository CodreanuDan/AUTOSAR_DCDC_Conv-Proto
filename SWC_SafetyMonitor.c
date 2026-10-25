#include "SWC_SafetyMonitor.h"
#include "SafetyEvents.h"
#include "Rte.h"


#define SAFE_VIN_FUSE_CHECK_ON_V   8.0f   /* below this the PB5 level is not a clear logic level */
#define SAFE_VIN_FUSE_CHECK_OFF_V  7.5f   /* hysteresis */

static uint8_t s_fuse_check_active = 0U;

/* Called every 10 ms in RUN, after SWC_SensorMeasurement_Runnable. v_in is measured BEFORE the fuse. */
void SWC_SafetyMonitor_Runnable(float v_in)
{
    uint8_t fuse_state;
    uint8_t causes = 0U;

    /* Enable the check only while Vin is high enough (with hysteresis) */
    if ((s_fuse_check_active == 0U) && (v_in >= SAFE_VIN_FUSE_CHECK_ON_V))
    {
        s_fuse_check_active = 1U;
    }    
	else if ((s_fuse_check_active != 0U) && (v_in < SAFE_VIN_FUSE_CHECK_OFF_V))
    {
        s_fuse_check_active = 0U;
    }

    if (s_fuse_check_active == 0U)
    {
        fuse_state = FUSE_STATE_UNKNOWN;                 /* no clear logic level */
    }
    else if (Rte_Read_FuseSense() == 0U)
    {
        fuse_state = FUSE_STATE_BLOWN;                   /* Vin present before the fuse, nothing after it */
        causes    |= SAFE_CAUSE_FUSE_BLOWN;
    }
    else
    {
        fuse_state = FUSE_STATE_OK;
    }

    Rte_Write_FuseState(fuse_state);
    Rte_Write_SafetyCauses(causes);
}