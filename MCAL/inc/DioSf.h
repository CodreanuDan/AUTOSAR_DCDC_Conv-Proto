/*
 * Filename: DioSf.h
 * @brief: Contains channel definitions and function prototypes for DIO Driver
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef DIO_SF_H
#define DIO_SF_H

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
 *            TYPE DEFINITIONS & MACROS
 *******************************************************/

typedef uint8_t Dio_ChannelType;
typedef uint8_t Dio_LevelType;

#ifndef STD_LOW
#define STD_LOW   0x00U
#endif

#ifndef STD_HIGH
#define STD_HIGH  0x01U
#endif

/* Definitions for Port identifiers */
#define DIO_PORT_B             0U
#define DIO_PORT_D             1U

/* Definitions for project specific DIO channels mapping port and pin */
#define DIO_CHANNEL_PWM_OC1A   ((Dio_ChannelType)((DIO_PORT_B << 4U) | PB1))
#define DIO_CHANNEL_PWM_OC1B   ((Dio_ChannelType)((DIO_PORT_B << 4U) | PB2))
#define DIO_CHANNEL_RELAY_IN   ((Dio_ChannelType)((DIO_PORT_D << 4U) | PD4))
#define DIO_CHANNEL_RELAY_OUT  ((Dio_ChannelType)((DIO_PORT_D << 4U) | PD5))

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Dio_ReadChannel
 * @brief Reads the physical state of a specific DIO pin across PORTB and PORTD.
 * @param: Dio_ChannelType ChannelId (Target channel identifier containing port and pin)
 * @return: Dio_LevelType (STD_HIGH if pin is HIGH, STD_LOW if pin is LOW)
 */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);

/**
 * Function name: Dio_WriteChannel
 * @brief Sets the physical output level of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target channel identifier)
 * @param: Dio_LevelType Level (STD_HIGH to activate, STD_LOW to deactivate)
 * @return: void
 */
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);

/**
 * Function name: Dio_FlipChannel
 * @brief Toggles the physical state of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target channel identifier)
 * @return: Dio_LevelType The new physical state of the pin after toggle.
 */
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* DIO_SF_H */