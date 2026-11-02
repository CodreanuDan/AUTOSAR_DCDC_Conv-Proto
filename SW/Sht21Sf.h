/******************************************************************************
 * @file    Sht21Sf.h
 * @brief   BSW driver for the Sensirion SHT21 temperature / humidity sensor
 * @details Non-blocking state machine. Call Sht21_MainFunction() every 10 ms
 *          (from IoHwAb). It alternates temperature and humidity measurements
 *          and publishes the converted values in 0.01 units:
 *              temp_c100 = 2712  ->  27.12 degC
 *              rh_c100   = 8000  ->  80.00 %RH
 *          Uses I2cSf (MCAL) for the bus and GptSf's g_tick_ms for timing.
 ******************************************************************************/
#ifndef SHT21SF_H
#define SHT21SF_H

#include "StdTypes.h"
#include <stdint.h>

/** Resets the state machine, marks data invalid. (I2c_Init is done by EcuM) */
void Sht21_Init(void);

/** Cyclic function, call every 10 ms. Never blocks. */
void Sht21_MainFunction(void);

/**
 * Function name: Sht21_GetData
 * @brief   Last good measurement (stays at the last value if sensor fails;
 *          check Sht21_IsValid()).
 * @param   temp_c100 : out, temperature in 0.01 degC (signed)
 * @param   rh_c100   : out, relative humidity in 0.01 %RH (0..10000)
 */
void Sht21_GetData(int16_t *temp_c100, uint16_t *rh_c100);

/** TRUE = data is fresh and trustworthy; FALSE = sensor not responding. */
uint8_t Sht21_IsValid(void);

#endif /* SHT21SF_H */
