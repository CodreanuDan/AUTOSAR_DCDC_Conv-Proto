/*
 * Filename: PwmSf.c
 * @brief: Contains the function definitions Timer 1 PWM Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "PwmSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/



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
    uint16_t top = (uint16_t)(F_CPU / (8UL * target_freq_hz) - 1UL);
    ICR1 = top;

    /* Configure Timer1 Control Registers:
     * COM1A1:1 & COM1B1:1 -> Non-inverting PWM on Channel A and Channel B
     * WGM13:1, WGM12:1, WGM11:1, WGM10:0 -> Fast PWM Mode 14 (TOP defined by ICR1)
     */
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);

    /* Set clock prescaler to 8 (CS11 = 1) */
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

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
		/* Any requested frequency below 16,000,000/(8×65536) ˜ 30.5Hz overflows the 16-bit TOP register again */
		if (new_freq_hz < 31U) 
		{ 
			new_freq_hz = 31U; 
		}

		/* Recalculate ICR1 TOP value for new frequency */
		ICR1 = (uint16_t)(F_CPU / (8UL * new_freq_hz) - 1UL);
	}
	else
	{
		/* Do nothing */
	}
}

/* Interfaces with AVR Registers */

uint16_t Pwm_ReadOcr1A(void)
{
    return OCR1A;
}

uint16_t Pwm_ReadOcr1B(void)
{
    return OCR1B;
}

uint16_t Pwm_ReadIcr1(void)
{
    return ICR1;
}

/**
 * Function name: Timer2_Pwm_Init
 * @brief Initializes Timer2 in Fast PWM Mode.
 * Fast PWM, TOP=0xFF, OC2A disconnected 
 * clk/256 -> 16MHz/(256*256) = 244 Hz.
 * @param: void
 * @return: void
 */
void Timer2_Pwm_Init(void)
{
	OCR2A  = 0U;
	TCCR2A = (1U << WGM21) | (1U << WGM20);   /* Fast PWM, TOP=0xFF, OC2A disconnected */
	TCCR2B = (1U << CS22) | (1U << CS21);     /* clk/256 -> 16MHz/(256*256) = 244 Hz */
}

void Pwm_Fan_SetDuty(uint8_t duty_pct)
{
	if (duty_pct > 100U) { duty_pct = 100U; }

	if (duty_pct == 0U)
	{
		TCCR2A &= (uint8_t)~(1U << COM2A1);   /* disconnect OC2A */
        PORTB  &= (uint8_t)~(1U << PB3);      /* pin LOW, no 1/256 spike */
        OCR2A   = 0U;
	}
	else
	{
		OCR2A   = (uint8_t)(((uint16_t)duty_pct * 255U + 50U) / 100U);
        TCCR2A |= (1U << COM2A1);             /* non-inverting PWM */
	}
}

uint8_t Pwm_Fan_GetDuty(void)
{
    if ((TCCR2A & (1U << COM2A1)) == 0U) { return 0U; }
    return (uint8_t)(((uint16_t)OCR2A * 100U + 127U) / 255U);
}
/************* END OF FUCTION DEFINITIONS ************/


/* End of PwmSf.c */