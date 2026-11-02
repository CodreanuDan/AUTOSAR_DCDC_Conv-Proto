/*
 * Filename: DioSf.c
 * @brief: Contains function definitions for DIO Driver
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

/* Project specific libs */
#include "DioSf.h"
#include <avr/interrupt.h>

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

/**
 * Function name: Dio_Kl15_InterruptInit
 * @brief Configures INT0 (PD2) to fire on any logical change, used solely to
 * wake the MCU from SLEEP_MODE_PWR_DOWN on a KL15 edge. Does not enable
 * global interrupts - caller must call sei() after full system init.
 * @param: void
 * @return: void
 */
void Dio_Kl15_InterruptInit(void)
{
    /* Switched from INT0 to Pin Change Interrupt: INT0's edge/"any change"
     * modes require a running I/O clock to detect the transition and do NOT
     * wake the MCU from SLEEP_MODE_PWR_DOWN. Pin change interrupts are
     * detected asynchronously and do reliably wake from Power-down. */
    PCICR  |= (1U << PCIE2);
    PCMSK2 |= (1U << PCINT18);   /* PD2 */
}

/* Body intentionally empty: this ISR exists only to wake the MCU from sleep.
 * Once awake, the cyclic IoHwAb_Kl15_MainFunction() re-reads and debounces
 * the pin properly - no state is handled here. */
ISR(PCINT2_vect) 
{

}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of DioSf.c */