/*
 * Filename: PwmSf.c
 * @brief: Contains the function definitions Timer 1 PWM Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "PwmSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/
static uint16_t s_pwm_top = 19999U; /* Default 100Hz at 16MHz (Prescaler = 8) */


/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *               Local Function Prototypes
 */

/*************End of Local Function Prototypes *********/


/********************************************************
 *             START OF FUCTION DEFINITIONS
 *******************************************************/
/**
 * Function name: Timer1_Pwm_Init
 * @brief Initializes Timer1 in Fast PWM Mode 14 (TOP = ICR1).
 * PWM mode on both channels, configures prescaler to 1, and sets initial frequency.
 * @param: uint16_t target_freq_hz (Target PWM frequency in Hertz)
 * @return: void
 */
void Timer1_Pwm_Init(uint16_t target_freq_hz)
{
    /* Calculate ICR1 TOP value: TOP = (F_CPU / (Prescaler * Target_Freq)) - 1 */
    uint16_t top = (uint16_t)(F_CPU / (1UL * target_freq_hz) - 1UL);
    ICR1 = top;

    /* Configure Timer1 Control Registers:
     * COM1A1:1 & COM1B1:1 -> Non-inverting PWM on Channel A and Channel B
     * WGM13:1, WGM12:1, WGM11:1, WGM10:0 -> Fast PWM Mode 14 (TOP defined by ICR1)
     */
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);

    /* CS10:1 -> No prescaling (Prescaler = 1) */
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);

    /* Set default safe duty cycle (98%) */
    OCR1A = (uint16_t)(top * 0.98f);
    OCR1B = (uint16_t)(top * 0.98f);
}

/**
 * Function name: Pwm_SetDutyCycle
 * @brief Updates duty cycle for Channel A (OC1A) or Channel B (OC1B).
 * Calculates OCR1A/OCR1B values based on the current ICR1 TOP value and percentage.
 * @param: uint8_t duty_a (Duty cycle percentage for Channel A: 0-100%)
 * @param: uint8_t duty_b (Duty cycle percentage for Channel B: 0-100%)
 * @return: void
 */
void Pwm_SetDutyCycle(uint8_t duty_a, uint8_t duty_b)
{
    uint16_t current_top = ICR1;

    /* Update compare registers based on current ICR1 TOP value */
    OCR1A = (uint16_t)(((uint32_t)current_top * duty_a) / 100U);
    OCR1B = (uint16_t)(((uint32_t)current_top * duty_b) / 100U);
}

/**
 * Function name: Pwm_SetFrequency
 * @brief Recalculates and updates ICR1 TOP value for a new target PWM frequency.
 * @param: uint16_t new_freq_hz (New PWM frequency in Hertz)
 * @return: void
 */
void Pwm_SetFrequency(uint16_t new_freq_hz)
{
    if (new_freq_hz > 0U)
    {
        /* Recalculate ICR1 TOP value for new frequency */
        ICR1 = (uint16_t)(F_CPU / (1UL * new_freq_hz) - 1UL);
    }
}

/************* END OF FUCTION DEFINITIONS ************/


/* End of PwmSf.c */