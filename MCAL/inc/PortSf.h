/*
 * Filename: PortSf.h
 * @brief: Contains the variables and function declaration for PORT Drivers 
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef PORT_SF_H
#define PORT_SF_H

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
 *            PIN CONFIGURATION MACROS
 *******************************************************/

/* PWM Output Pins (PORTB) */
#define PORT_PIN_PWM_OC1A   PB1
#define PORT_PIN_PWM_OC1B   PB2

/* Relay Output Pins (PORTD) */
#define PORT_PIN_RELAY_IN   PD4
#define PORT_PIN_RELAY_OUT  PD5

/*******************************************************
 *            START OF VARIABLE DECLARATIONS
 *******************************************************/



/************* END OF VARIABLE DECLARATIONS ************/

/*******************************************************
 *             START OF FUCTION PROTOTYPES
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
void Port_Init(void);


/************* END OF FUCTION PROTOTYPES ************/


/* C++ Guards for normal inclusion */
#ifdef __cplusplus
}
#endif

#endif /* PORT_SF_H */