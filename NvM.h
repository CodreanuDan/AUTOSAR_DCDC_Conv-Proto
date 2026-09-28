/*
 * Filename: NvM.h
 * @brief: Contains type definitions and function prototypes for Non-Volatile Memory Manager (NvM)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef NVM_H
#define NVM_H

/* Core libs */
#include <stdint.h>
#include <stdbool.h>

/* Project specific libs */
#include "StdTypes.h"

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *            START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: NvM_Init
 * @brief Initializes the Non-Volatile Memory manager and verifies EEPROM block integrity.
 * @param: void
 * @return: void
 */
void NvM_Init(void);

/**
 * Function name: NvM_ReadAll
 * @brief Reads persistent calibration data, configurations, and DTC status from EEPROM into RAM.
 * @param: void
 * @return: void
 */
void NvM_ReadAll(void);

/**
 * Function name: NvM_WriteAll
 * @brief Writes current runtime calibration data, configuration parameters, and DTC history to EEPROM.
 * @param: void
 * @return: void
 */
void NvM_WriteAll(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* NVM_H */