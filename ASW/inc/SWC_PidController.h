/*
 * Filename: SWC_PidController.h
 * @brief: Contains type definitions, converter state macros, and function prototypes for SWC_PidController
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#ifndef SWC_PID_CONTROLLER_H
#define SWC_PID_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

/*******************************************************
 *            TYPE DEFINITIONS & MACROS
 *******************************************************/
/* Converter Operating States */
#define CONVERTER_STATE_PASSTHROUGH  0U
#define CONVERTER_STATE_BOOST        1U
#define CONVERTER_STATE_BUCK         2U

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/
void SWC_PidController_Runnable(float v_in, float v_out);

#endif /* SWC_PID_CONTROLLER_H */