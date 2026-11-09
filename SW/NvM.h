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
 *            TYPE DEFINITIONS & MACROS
 *******************************************************/

#define NVM_LEN_IDENT    40U
#define NVM_LEN_CODING   13U

typedef enum
{
    NVM_BLK_IDENT = 0,
    NVM_BLK_CODING,
    NVM_NUM_BLOCKS
} NvM_BlockIdType;

#define NVM_E_OK        0U
#define NVM_E_INVALID   1U   /* blank EEPROM, wrong layout version or CRC error */

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

/* Reads one managed block. Returns NVM_E_OK only if layout version and CRC match. */
uint8_t NvM_ReadBlock(NvM_BlockIdType id, uint8_t *data);

/* Writes one managed block with layout version and CRC. */
void NvM_WriteBlock(NvM_BlockIdType id, const uint8_t *data);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* NVM_H */