/*
 * Filename: PortSf.c
 * @brief: Contains the function definitions for PORT Drivers
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "PortSf.h"

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
 * Function name: Port_Init
 * @brief Initializes all microcontroller port pins used by the system.
 * Configures PB1 and PB2 as digital outputs for PWM generation,
 * configures PD4 and PD5 as digital outputs for input/output relay control,
 * and sets the default initial output levels for relays to OFF (STD_LOW).
 * @param: void
 * @return: void
 */
void Port_Init(void)
{
    /* Configure PWM pins PB1 (OC1A) and PB2 (OC1B) as digital outputs */
    DDRB |= (1U << PORT_PIN_PWM_OC1A) | (1U << PORT_PIN_PWM_OC1B);

    /* Configure Relay pins PD4 and PD5 as digital outputs */
    DDRD |= (1U << PORT_PIN_RELAY_IN) | (1U << PORT_PIN_RELAY_OUT);

    /* Set default initial state: disable relays (STD_LOW) */
    PORTD &= ~((1U << PORT_PIN_RELAY_IN) | (1U << PORT_PIN_RELAY_OUT));
}

/************* END OF FUCTION DEFINITIONS ************/


/* End of PortSf.c */