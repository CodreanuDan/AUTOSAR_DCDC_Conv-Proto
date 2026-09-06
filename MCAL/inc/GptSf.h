/*
 * Filename: GptSf.h
 * @brief: Contains the variables and function declaration for General Purpose Hardware Timer (Timer0) Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef GPT_SF_H
#define GPT_SF_H

/* Core libs */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

/* Project specific libs */
#include "StdTypes.h"

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************
 *            START OF VARIABLE DECLARATIONS
 *******************************************************/

/* Simple 1ms tick for periodic sampling - Incremented by Timer0 Compare Match A interrupt every 1 millisecond.*/
extern volatile uint32_t g_tick_ms;

/************* END OF VARIABLE DECLARATIONS ************/

/*******************************************************
 *             START OF FUCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Timer0_TickInit
 * @brief Initializes Timer0 in CTC mode to generate a periodic 1ms interrupt.
 * Configures Timer0 CTC mode (WGM01), sets the output compare register OCR0A 
 * for 1ms tick duration (assuming 16 MHz clock and prescaler 64), enables 
 * the Compare Match A interrupt, and starts the timer clock.
 * @param: void
 * @return: void
 */
void Timer0_TickInit(void);

/************* END OF FUCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* GPT_SF_H */