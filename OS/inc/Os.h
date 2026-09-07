/*
 * Filename: Os.h
 * @brief: Contains operating system scheduler flags and hook prototypes
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#ifndef OS_H
#define OS_H

#include <stdint.h>
#include "StdTypes.h"

/*******************************************************
 *            EXTERNAL VARIABLE DECLARATIONS
 *******************************************************/
extern volatile uint8_t g_flag_task_10ms;
extern volatile uint8_t g_flag_task_100ms;
extern volatile uint8_t g_flag_task_500ms;

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/
void Os_TimerTick_Hook(void);

#endif /* OS_H */