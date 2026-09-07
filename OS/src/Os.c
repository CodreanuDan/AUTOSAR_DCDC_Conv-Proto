/*
 * Filename: OS_Scheduler.c
 * @brief: Contains operating system timer hook and task scheduling flags
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include <avr/interrupt.h>
#include "Rte.h"
#include "StdTypes.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/
volatile uint8_t g_flag_task_10ms  = 0;
volatile uint8_t g_flag_task_100ms = 0;
volatile uint8_t g_flag_task_500ms = 0;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

/*
 * Function name: Os_TimerTick_Hook
 * @brief System tick handler called every 1ms from Timer0 interrupt. Sets cyclic task flags.
 * @param: void
 * @return: void
 */
void Os_TimerTick_Hook(void)
{
    static uint16_t timer_10ms = 0;
    static uint16_t timer_100ms = 0;
    static uint16_t timer_500ms = 0;

    timer_10ms++;
    timer_100ms++;
    timer_500ms++;

    if (timer_10ms >= 10)   { timer_10ms = 0;   g_flag_task_10ms = 1; }
    if (timer_100ms >= 100) { timer_100ms = 0; g_flag_task_100ms = 1; }
    if (timer_500ms >= 500) { timer_500ms = 0; g_flag_task_500ms = 1; }
}

ISR(TIMER0_COMPA_vect)
{
    Os_TimerTick_Hook();
}
/************* END OF FUNCTION DEFINITIONS ************/