/*
 * Filename: WdgSf.c
 * @brief: Contains function definitions for Watchdog Driver
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "WdgSf.h"

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: Wdg_Init
 * @brief Initializes the Watchdog Timer hardware module with a specified timeout.
 * @param: uint8_t timeout_val (AVR Watchdog timeout preset, e.g., WDTO_2S, WDTO_1S)
 * @return: void
 */
void Wdg_Init(uint8_t timeout_val)
{
    /* Enable WDT with requested timeout prescaler */
    wdt_enable(timeout_val);
}

/**
 * Function name: Wdg_Trigger
 * @brief Resets the hardware Watchdog Timer counter to prevent system reset.
 * Must be called periodically within the main system execution loop.
 * @param: void
 * @return: void
 */
void Wdg_Trigger(void)
{
    /* Service the hardware Watchdog timer */
    wdt_reset();
}

/**
 * Function name: Wdg_Disable
 * @brief Disables the hardware Watchdog Timer.
 * @param: void
 * @return: void
 */
void Wdg_Disable(void)
{
    /* Safely turn off Watchdog Timer using core library helper */
    wdt_disable();
}

/**
 * Function name: Wdg_PerformReset
 * @brief Triggers an immediate ECU software reset via Watchdog timeout.
 * Used by EcuM via DcmSf for UDS ECU Reset Service (0x11 02).
 * @param: void
 * @return: void
 */
void Wdg_PerformReset(void)
{
    /* Configure minimum possible Watchdog timeout (15 milliseconds) */
    wdt_enable(WDTO_15MS);

    /* Enter infinite loop and wait for Watchdog reset to strike */
    while (1)
    {
        /* Do nothing, force WDT timeout */
    }
}

/************* END OF FUNCTION PROTOTYPES ************/

/* End of WdgSf.c */