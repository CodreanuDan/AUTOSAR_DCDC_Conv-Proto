/*
 * Filename: DioSf.c
 * @brief: Contains function definitions for DIO Driver
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "DioSf.h"

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: Dio_ReadChannel
 * @brief Reads the physical state of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target pin identifier, e.g., PD4, PD5)
 * @return: Dio_LevelType (STD_HIGH if pin is HIGH, STD_LOW if pin is LOW)
 */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    Dio_LevelType level = STD_LOW;

    /* Read the physical input/output state from PIND register */
    if ((PIND & (1U << ChannelId)) != 0U)
    {
        level = STD_HIGH;
    }

    return level;
}

/**
 * Function name: Dio_WriteChannel
 * @brief Sets the physical output level of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target pin identifier, e.g., PD4, PD5)
 * @param: Dio_LevelType Level (STD_HIGH to activate, STD_LOW to deactivate)
 * @return: void
 */
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    if (Level == STD_HIGH)
    {
        PORTD |= (1U << ChannelId);
    }
    else
    {
        PORTD &= ~(1U << ChannelId);
    }
}

/**
 * Function name: Dio_FlipChannel
 * @brief Toggles the physical state of a specific DIO pin.
 * @param: Dio_ChannelType ChannelId (Target pin identifier, e.g., PD4, PD5)
 * @return: Dio_LevelType The new physical state of the pin after toggle.
 */
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId)
{
    /* Writing 1 to PIND register bit automatically toggles PORTD bit in AVR hardware */
    PIND |= (1U << ChannelId);

    return Dio_ReadChannel(ChannelId);
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of DioSf.c */