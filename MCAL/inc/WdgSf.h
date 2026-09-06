/*
 * Filename: WdgSf.h
 * @brief: Contains register definitions and function prototypes for Watchdog Driver
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef WDG_SF_H
#define WDG_SF_H

/* Core libs */
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdbool.h>

/* Project specific libs */
#include "StdTypes.h"

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Wdg_Init
 * @brief Initializes the Watchdog Timer hardware module with a specified timeout.
 * @param: uint8_t timeout_val (AVR Watchdog timeout preset, e.g., WDTO_2S, WDTO_1S, WDTO_15MS)
 * @return: void
 */
void Wdg_Init(uint8_t timeout_val);

/**
 * Function name: Wdg_Trigger
 * @brief Resets the hardware Watchdog Timer counter to prevent system reset.
 * Must be called periodically within the main system execution loop.
 * @param: void
 * @return: void
 */
void Wdg_Trigger(void);

/**
 * Function name: Wdg_Disable
 * @brief Disables the hardware Watchdog Timer.
 * @param: void
 * @return: void
 */
void Wdg_Disable(void);

/**
 * Function name: Wdg_PerformReset
 * @brief Triggers an immediate ECU software reset via Watchdog timeout.
 * Used by EcuM via DcmSf for UDS ECU Reset Service (0x11 02).
 * @param: void
 * @return: void
 */
void Wdg_PerformReset(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* WDG_SF_H */