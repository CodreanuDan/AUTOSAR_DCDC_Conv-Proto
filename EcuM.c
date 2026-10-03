/*
 * Filename: EcuM.c
 * @brief: Contains state machine definitions and initialization flow for EcuM
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

/* Core libs */
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* Project specific libs */
#include "EcuM.h"
#include "PortSf.h"
#include "DioSf.h"
#include "AdcSf.h"
#include "PwmSf.h"
#include "GptSf.h"
#include "UartSf.h"
#include "WdgSf.h"
#include "NvM.h"
#include "DemSf.h"
#include "ComSf.h"
#include "DcmSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

static EcuM_StateType s_ecum_state       = ECUM_STATE_STARTUP;
static EcuM_ResetType s_ecum_reset_req   = ECUM_RESET_NONE;

static bool s_ecum_sleep_req = false;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: EcuM_Init
 * @brief Executes startup sequence: initializes MCAL drivers, NvM, BSW, and RTE.
 * @param: void
 * @return: void
 */
void EcuM_Init(void)
{
    /* 1. Initialize Microcontroller Ports */
    Port_Init();

	/* KL15 Manager */
	Dio_Kl15_InterruptInit();

    /* 3. Initialize Diagnostic Event Manager */
    Dem_Init();

    /* 4. Initialize Non-Volatile Memory & Restore Persistent Configurations */
    NvM_Init();
    NvM_ReadAll();

    /* 5. Initialize Communication and Diagnostic Modules */
    Uart_Init(DEFAULT_BAUD_RATE);
    Dcm_Init();

    /* 6. Initialize Hardware Timers and Drivers */
    Adc_Init();
    Timer0_TickInit();
    Timer1_Pwm_Init(Rte_Read_TargetFrequency()); /* Default initial PWM frequency: 100Hz */

    /* 7. Initialize Watchdog Timer (2 seconds timeout) */
    Wdg_Init(WDTO_2S);

    /* 8. Enable Global Interrupts */
    sei();

    /* 9. Transition to RUN state */
    s_ecum_state = ECUM_STATE_RUN;
}

/**
 * Function name: EcuM_MainFunction
 * @brief Central state machine scheduler running main background loops.
 * @param: void
 * @return: void
 */
void EcuM_MainFunction(void)
{
    switch (s_ecum_state)
    {
        case ECUM_STATE_STARTUP:
            /* Handled entirely within EcuM_Init() */
            break;

        case ECUM_STATE_RUN:
            /* 1. Service the Watchdog timer periodically */
            Wdg_Trigger();

            /* 2. Check if a software reset or shutdown was requested via DCM */
            if (s_ecum_reset_req != ECUM_RESET_NONE)
            {
                s_ecum_state = ECUM_STATE_SHUTDOWN;
            }
			else if (s_ecum_sleep_req == true)
			{
				s_ecum_state = ECUM_STATE_SHUTDOWN;
			}
            break;

        case ECUM_STATE_SHUTDOWN:
            /* 1. Disable PWM outputs and safely set relays to LOW */
            Pwm_SetDutyCycle(0U, 0U);
            Dio_WriteChannel(DIO_CHANNEL_RELAY_IN, STD_LOW);
            Dio_WriteChannel(DIO_CHANNEL_RELAY_OUT, STD_LOW);

            /* 2. Save all persistent calibrations and fault memories to EEPROM via NvM */
            NvM_WriteAll();

            /* 3. Perform hardware reset if requested via UDS 0x11 */
            if (s_ecum_reset_req == ECUM_RESET_SOFT)
            {
                Wdg_PerformReset();
            }

			else if (s_ecum_sleep_req == true)
			{
				s_ecum_sleep_req = false;
				s_ecum_state = ECUM_STATE_SLEEP;
			}
			break;

        case ECUM_STATE_SLEEP:
			wdt_disable();   /* don't let the watchdog fire while we're asleep */

			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_enable();
			sleep_cpu();        /* halts here until INT0 (KL15 edge) wakes it */
			sleep_disable();

			Wdg_Init(WDTO_2S);  /* re-arm watchdog immediately on wake */

			s_ecum_state = ECUM_STATE_RUN;
			break;

        default:
            s_ecum_state = ECUM_STATE_RUN;
            break;
    }
}

/**
 * Function name: EcuM_SetResetRequest
 * @brief Sets the requested reset type flag (called by DcmSf for UDS 0x11).
 * @param: EcuM_ResetType reset_type
 * @return: void
 */
void EcuM_SetResetRequest(EcuM_ResetType reset_type)
{
    if (s_ecum_state == ECUM_STATE_RUN)
    {
        s_ecum_reset_req = reset_type;
    }
}

/**
 * Function name: EcuM_GetState
 * @brief Returns the current operational state of the ECU.
 * @param: void
 * @return: EcuM_StateType
 */
EcuM_StateType EcuM_GetState(void)
{
    return s_ecum_state;
}

void EcuM_RequestSleep(void)
{
    if (s_ecum_state == ECUM_STATE_RUN)
    {
        s_ecum_sleep_req = true;
    }
}

void EcuM_RequestWake(void)
{
    s_ecum_sleep_req = false;   /* cancels a pending sleep if KL15 flickers back ON in time */
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of EcuM.c */