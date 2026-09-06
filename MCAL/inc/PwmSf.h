/*
 * Filename: PwmSf.h
 * @brief: Contains the variables and function declaration for Timer1 Pwm Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef PWM_SF_H
#define PWM_SF_H

/* Core libs */
#include <avr/io.h>
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



/************* END OF VARIABLE DECLARATIONS ************/

/*******************************************************
 *             START OF FUCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Timer1_Pwm_Init
 * @brief Initializes Timer1 in Fast PWM Mode 14 (TOP = ICR1).
 * Configures PB1 (OC1A) and PB2 (OC1B) pins as outputs, sets non-inverting 
 * PWM mode on both channels, configures prescaler to 1, and sets initial frequency.
 * @param: uint16_t target_freq_hz (Target PWM frequency in Hertz)
 * @return: void
 */
void Timer1_Pwm_Init(uint16_t target_freq_hz);

/**
 * Function name: Pwm_SetDutyCycle
 * @brief Updates duty cycle for Channel A (OC1A) or Channel B (OC1B).
 * Calculates OCR1A/OCR1B values based on the current ICR1 TOP value and percentage.
 * @param: uint8_t duty_a (Duty cycle percentage for Channel A: 0-100%)
 * @param: uint8_t duty_b (Duty cycle percentage for Channel B: 0-100%)
 * @return: void
 */
void Pwm_SetDutyCycle(uint8_t duty_a, uint8_t duty_b);

/**
 * Function name: Pwm_SetFrequency
 * @brief Recalculates and updates ICR1 TOP value for a new target PWM frequency.
 * @param: uint16_t new_freq_hz (New PWM frequency in Hertz)
 * @return: void
 */
void Pwm_SetFrequency(uint16_t new_freq_hz);

/************* END OF FUCTION PROTOTYPES ************/


/* C++ Guards for normal inclusion */
#ifdef __cplusplus
}
#endif

#endif /* PWM_SF_H */