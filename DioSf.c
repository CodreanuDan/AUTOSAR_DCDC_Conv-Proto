/*
 * Filename: DioSf.c
 * @brief: Contains function definitions for DIO Driver
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

/* Project specific libs */
#include "DioSf.h"

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: Dio_ReadChannel
 * @brief Reads the physical state of a specific DIO pin across PORTB and PORTD.
 * @param: Dio_ChannelType ChannelId (Target channel identifier containing port and pin)
 * @return: Dio_LevelType (STD_HIGH if pin is HIGH, STD_LOW if pin is LOW)
 */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    Dio_LevelType level = STD_LOW;
    uint8_t port = (uint8_t)(ChannelId >> 4U);
    uint8_t pin  = (uint8_t)(ChannelId & 0x0FU);

    if (port == DIO_PORT_B)
    {
        if ((PINB & (1U << pin)) != 0U)
        {
            level = STD_HIGH;
        }
    }
    else if (port == DIO_PORT_D)
    {
        if ((PIND & (1U << pin)) != 0U)
        {
            level = STD_HIGH;
        }
    }
    else
    {
        /* Invalid port descriptor */
    }

    return level;
}

/**
 * Function name: Dio_WriteChannel
 * @brief Sets the physical output level of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target channel identifier)
 * @param: Dio_LevelType Level (STD_HIGH to activate, STD_LOW to deactivate)
 * @return: void
 */
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    uint8_t port = (uint8_t)(ChannelId >> 4U);
    uint8_t pin  = (uint8_t)(ChannelId & 0x0FU);

    if (port == DIO_PORT_B)
    {
        if (Level == STD_HIGH)
        {
            PORTB |= (1U << pin);
        }
        else
        {
            PORTB &= ~(1U << pin);
        }
    }
    else if (port == DIO_PORT_D)
    {
        if (Level == STD_HIGH)
        {
            PORTD |= (1U << pin);
        }
        else
        {
            PORTD &= ~(1U << pin);
        }
    }
    else
    {
        /* Invalid port descriptor */
    }
}

/**
 * Function name: Dio_FlipChannel
 * @brief Toggles the physical state of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target channel identifier)
 * @return: Dio_LevelType The new physical state of the pin after toggle.
 */
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId)
{
    uint8_t port = (uint8_t)(ChannelId >> 4U);
    uint8_t pin  = (uint8_t)(ChannelId & 0x0FU);

    if (port == DIO_PORT_B)
    {
        PINB |= (1U << pin);
    }
    else if (port == DIO_PORT_D)
    {
        PIND |= (1U << pin);
    }
    else
    {
        /* Invalid port descriptor */
    }

    return Dio_ReadChannel(ChannelId);
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of DioSf.c */