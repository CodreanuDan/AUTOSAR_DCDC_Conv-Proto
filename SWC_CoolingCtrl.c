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

#define COOL_T_C100_BAND1       2500    /* 25.00 degC: band 0 -> 1 */
#define COOL_T_C100_BAND2       5000    /* 50.00 degC: band 1 -> 2 */
#define COOL_T_C100_BAND3       8000    /* 80.00 degC: band 2 -> 3 */
#define COOL_HYST_C100          200     /* 2.00 degC, applied when stepping down */
#define COOL_SENSOR_GRACE_CALLS 100U    /* 100 x 10 ms = 1 s of invalid before FAILSAFE */

static const int16_t s_band_up[3]    = { COOL_T_C100_BAND1, COOL_T_C100_BAND2, COOL_T_C100_BAND3 };
static const uint8_t s_band_duty[4]  = { 25U, 50U, 75U, 90U };

static uint8_t s_band         = 0U;    /* current band 0..3 */
static uint8_t s_band_valid   = 0U;    /* 0 = no band chosen yet (first valid sample picks it) */
static uint8_t s_invalid_cnt  = 0U;

/************* END OF STATIC LOCAL VARIABLES ***********/

/*******************************************************
 *            LOCAL FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Auto curve. Returns the duty; 
 *failsafe = 1 if the SHT21 has been invalid longer than the grace time.
 */
static uint8_t Cool_AutoDuty(uint8_t *failsafe);

/*********** END OF LOCAL FUNCTION PROTOTYPES **********/

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

static uint8_t Cool_AutoDuty(uint8_t *failsafe)
{
	int16_t t = Rte_Read_AmbTemp();     /* 0.01 degC */

	*failsafe = 0U;

	if (Rte_Read_Sht21Valid() == 0U)
	{
		if (s_invalid_cnt < COOL_SENSOR_GRACE_CALLS) 
		{ 
			s_invalid_cnt++; 
		}

		if (s_invalid_cnt >= COOL_SENSOR_GRACE_CALLS)
		{
			*failsafe    = 1U;
			s_band_valid = 0U;                /* re-pick the band when the sensor comes back */
			return FAN_MAX_DUTY;
		}
		return s_band_duty[s_band];      	  /* grace: hold the current band */

	}

	s_invalid_cnt = 0U;

	if (s_band_valid == 0U)               /* first valid sample: pick the band directly, no hysteresis */
	{
		s_band = 0U;
		while ((s_band < 3U) && (t >= s_band_up[s_band])) 
		{ 
			s_band++; 
		}
		s_band_valid = 1U;
	}
	else if ((s_band < 3U) && (t >= s_band_up[s_band]))
	{
		s_band++;                         /* step up */
	}
	else if ((s_band > 0U) && (t < (int16_t)(s_band_up[s_band - 1U] - COOL_HYST_C100)))
	{
		s_band--;                         /* step down, with hysteresis */
	}

	return s_band_duty[s_band];
}

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
        if (duty > FAN_MAX_DUTY) 
		{ 
			duty = FAN_MAX_DUTY; 
		}
        state = (duty > 0U) ? COOL_RUN : COOL_OFF;
    }
    else                                       /* 3. auto */
    {
        uint8_t failsafe;
        duty  = Cool_AutoDuty(&failsafe);
        state = (failsafe != 0U) ? COOL_FAILSAFE : COOL_RUN;
    }

    Rte_Write_FanDuty(duty);
    Rte_Write_CoolState(state);
}

/************* END OF FUNCTION DEFINITIONS ************/