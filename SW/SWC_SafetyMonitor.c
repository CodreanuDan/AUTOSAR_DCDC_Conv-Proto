#include "SWC_SafetyMonitor.h"
#include "SafetyEvents.h"
#include "Rte.h"

/*******************************************************
 *            STATIC LOCAL VARIABLES (ASW Cache)
 *******************************************************/

#define SAFE_VIN_FUSE_CHECK_ON_V   8.0f   /* below this the PB5 level is not a clear logic level */
#define SAFE_VIN_FUSE_CHECK_OFF_V  7.5f   /* hysteresis */

#define SAFE_NUM_CAUSES        8U
#define SAFE_RETRY_MAX         3U       /* shutdowns ...                    */
#define SAFE_RETRY_WINDOW_CALLS 6000U   /* ... within 60 s => stay latched  */

typedef struct {
    uint8_t  mask;
    uint16_t set_calls;   /* 10 ms calls the condition must hold to become active */
    uint16_t clr_calls;   /* 10 ms calls it must be gone to become inactive (= hold time) */
} Safe_CauseCfg;

static const Safe_CauseCfg k_cause[SAFE_NUM_CAUSES] =
{
    { SAFE_CAUSE_OVERCURRENT_IN,   20U, 200U },  /* 200 ms / hold 2 s */
    { SAFE_CAUSE_OVERCURRENT_OUT,  50U, 100U },
    { SAFE_CAUSE_OVERVOLT_IN,      50U, 100U },
    { SAFE_CAUSE_UNDERVOLT_IN,     50U, 100U },
    { SAFE_CAUSE_OVERVOLT_OUT,     50U, 100U },
    { SAFE_CAUSE_FUSE_BLOWN,        1U, 100U },  /* already debounced 100 ms in IoHwAb */
    { SAFE_CAUSE_OVERTEMP_CRIT,   100U, 500U },  /* 1 s / hold 5 s */
    { SAFE_CAUSE_OVERVOLT_IN_CRIT, 20U, 200U }
};

static float   s_vin, s_vout, s_iin, s_iout;
static uint8_t s_fuse_check_active = 0U;
static uint8_t s_fuse_state = FUSE_STATE_UNKNOWN;

static uint16_t s_cnt[SAFE_NUM_CAUSES];
static uint8_t  s_active   = 0U;
static uint8_t  s_latched  = 0U;
static uint8_t  s_escalated = 0U;
static uint8_t  s_trips = 0U;
static uint16_t s_win_calls = 0U;
static uint8_t  s_restarts = 0U;

/************* END OF STATIC LOCAL VARIABLES ***********/

/*******************************************************
 *            LOCAL FUNCTION PROTOTYPES
 *******************************************************/
static void Safe_UpdateFuse(void);
/* Raw condition with hysteresis. was = cause currently active. */
static uint8_t Safe_Cond(uint8_t idx, uint8_t was);
/*********** END OF LOCAL FUNCTION PROTOTYPES **********/

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

static void Safe_UpdateFuse(void)
{
    if ((s_fuse_check_active == 0U) && (s_vin >= SAFE_VIN_FUSE_CHECK_ON_V))       
	{
		s_fuse_check_active = 1U;
	}
    else if ((s_fuse_check_active != 0U) && (s_vin < SAFE_VIN_FUSE_CHECK_OFF_V))  
	{
		s_fuse_check_active = 0U;
	}

    if (s_fuse_check_active == 0U)             
	{
		s_fuse_state = FUSE_STATE_UNKNOWN;
	}
    else if (Rte_Read_FuseSense() == 0U)       
	{
		s_fuse_state = FUSE_STATE_BLOWN;
	}
    else                                       
	{
		s_fuse_state = FUSE_STATE_OK;
	}

    Rte_Write_FuseState(s_fuse_state);
}

/* Raw condition with hysteresis. was = cause currently active. */
static uint8_t Safe_Cond(uint8_t idx, uint8_t was)
{
    float sp, lim, lim2;
    int16_t t;

    switch (idx)
    {
		/* SAFE_CAUSE_OVERCURRENT_IN */
        case 0U: 
			return (s_iin  > (was ? 7.0f : 8.0f)) ? 1U : 0U;
		/* SAFE_CAUSE_OVERCURRENT_OUT */
        case 1U: 
			return (s_iout > (was ? 7.0f : 8.0f)) ? 1U : 0U;
		/* SAFE_CAUSE_OVERVOLT_IN */
        case 2U: 
			return (s_vin  > (was ? 15.5f : 16.0f)) ? 1U : 0U;
		/* SAFE_CAUSE_UNDERVOLT_IN */
        case 3U:
			{
				if (s_vin < 2.0f) 
				{
					/* no supply at all: not a brown-out */
					return 0U;                    
				}
				return (s_vin < (was ? 9.5f : 9.0f)) ? 1U : 0U;
			}
		/* SAFE_CAUSE_OVERVOLT_OUT */
        case 4U:
			{
				sp   = (float)Rte_Read_PidTargetSetpoint();
				lim  = sp * 1.2f;
				lim2 = sp + 3.0f;

				if (lim2 > lim) 
				{
					lim = lim2;
				}

				return (s_vout > (was ? (lim - 1.0f) : lim)) ? 1U : 0U;
			}
		/* SAFE_CAUSE_FUSE_BLOWN */
        case 5U: 
			return (s_fuse_state == FUSE_STATE_BLOWN) ? 1U : 0U;
		/* SAFE_CAUSE_OVERTEMP_CRIT */
        case 6U:
			{
				if (Rte_Read_Sht21Valid() == 0U) 
				{
					return was;    /* no data: keep state */
				}
				t = Rte_Read_AmbTemp();                          /* 0.01 degC */
				return (t >= (was ? 8000 : 10000)) ? 1U : 0U;
			}
		/* SAFE_CAUSE_OVERVOLT_IN_CRIT */
        case 7U: 
			return (s_vin > (was ? 23.0f : 24.0f)) ? 1U : 0U;
        default: 
			return 0U;
    }
}

/* Called every 10 ms in RUN, after SWC_SensorMeasurement_Runnable. v_in, i_in is measured BEFORE the fuse. */
void SWC_SafetyMonitor_Runnable(float v_in, float v_out, float i_in, float i_out)
{
	uint8_t i, cond, act, enable, shut, state;

	s_vin = v_in; s_vout = v_out; s_iin = i_in; s_iout = i_out;
	Safe_UpdateFuse();

	/* 1. Debounced cause detection (table driven) */
	for (i = 0U; i < SAFE_NUM_CAUSES; i++)
	{
		act  = (uint8_t)((s_active & k_cause[i].mask) != 0U);
		cond = Safe_Cond(i, act);
	}

	if (act == 0U)
	{
		if (cond != 0U) 
		{ 
			if (++s_cnt[i] >= k_cause[i].set_calls) 
			{ 
				s_active |= k_cause[i].mask; s_cnt[i] = 0U; 
			} 
		}
		else            
		{ 
			s_cnt[i] = 0U; 
		}
	}
	else
	{
		if (cond == 0U) 
		{ 
			if (++s_cnt[i] >= k_cause[i].clr_calls) 
			{ 
				s_active &= (uint8_t)~k_cause[i].mask; s_cnt[i] = 0U; 
			} 
		}
		else            
		{ 
			s_cnt[i] = 0U; 
		}
	}

	/* 2. Reaction + recovery */
    enable = Rte_Read_ConverterEnable();
	shut   = (uint8_t)(s_active & SAFE_SHUTDOWN_MASK);

	if (s_trips != 0U)                                   /* retry window */
    {
        if (++s_win_calls >= SAFE_RETRY_WINDOW_CALLS) 
		{ 
			s_trips = 0U; 
			s_win_calls = 0U; 
		}
    }

    if (enable != 0U)
    {
        if (shut != 0U)                                  /* TRIP */
        {
            enable = 0U;
            if (s_trips == 0U) s_win_calls = 0U;
            s_trips++;
            if (s_trips >= SAFE_RETRY_MAX) s_escalated = 1U;
        }
    }
    else
    {
        if (Rte_Read_SafetyResetReq() != 0U)             /* manual reset (DCM checked the condition too) */
        {
            if (shut == 0U) 
			{ 
				enable = 1U; 
				s_latched = 0U; 
				s_escalated = 0U; 
				s_trips = 0U; 
			}
        }
        else if ((Rte_Read_RecoveryMode() == SAFE_RECOVERY_AUTO) && (s_escalated == 0U) && (shut == 0U))
        {
            enable = 1U; s_latched = 0U; s_restarts++;
        }
    }
    Rte_Write_SafetyResetReq(0U);

    if (enable == 0U) s_latched |= shut;

    /* 3. Publish */
    if (enable == 0U)          
	{
		state = SAFE_STATE_SHUTDOWN;
	}
    else if (s_active != 0U)   
	{
		state = SAFE_STATE_DEGRADED;
	}
    else                       
	{
		state = SAFE_STATE_OK;
	}

    Rte_Write_ConverterEnable(enable);
    Rte_Write_SafetyState(state);
    Rte_Write_SafetyCauses(s_active);
    Rte_Write_SafetyLatched(s_latched);
    Rte_Write_SafetyRestarts(s_restarts);

}
/************* END OF FUNCTION DEFINITIONS ************/