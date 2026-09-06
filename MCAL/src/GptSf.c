/*
 * Filename: GptSf.c
 * @brief: Contains the function definitions for Timer0 Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "GptSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/* Simple 1ms tick for periodic sampling - Incremented by Timer0 Compare Match A interrupt every 1 millisecond.*/
extern volatile uint32_t g_tick_ms = 0;

/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *               Local Function Prototypes
 */
static void Timer0_ISR_Routine(void);
/*************End of Local Function Prototypes *********/


/********************************************************
 *             START OF FUCTION DEFINITIONS
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
void Timer0_TickInit(void)
{
    /* Set Timer0 to CTC (Clear Timer on Compare Match) mode */
    TCCR0A = (1 << WGM01);

    /* Load compare value for 1ms: F_CPU / (Prescaler * Target_Freq) - 1 = 16MHz / (64 * 1000Hz) - 1 = 249 */
    OCR0A  = 249;

    /* Set clock prescaler to 64 (CS01 = 1, CS00 = 1) and start Timer0 */
    TCCR0B = (1 << CS01) | (1 << CS00);

    /* Enable Timer0 Output Compare Match A Interrupt */
    TIMSK0 = (1 << OCIE0A);
}

/**
 * @brief Interrupt Service Routine (ISR) for Timer0 Compare Match A.
 * Automatically executed every 1ms when Timer0 counter matches OCR0A register.
 */
static void Timer0_ISR_Routine(void)
{
    /* Increment system tick counter */
    g_tick_ms++;
}

/************* END OF FUCTION DEFINITIONS ************/

/******************* START OF ISR ********************/

ISR(TIMER0_COMPA_vect) 
{
    Gpt_ISR_Routine();
}

/********************** END OF ISR ********************/

/* End of GptSf.c */