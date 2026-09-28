/*
 * Filename: EcuM.h
 * @brief: Contains state definitions and function prototypes for ECU State Manager (EcuM)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef ECUM_H
#define ECUM_H

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

/**
 * @brief ECU Operating States
 */
typedef enum {
    ECUM_STATE_STARTUP,     /* System initialization phase */
    ECUM_STATE_RUN,         /* Normal operational run state */
    ECUM_STATE_SHUTDOWN,    /* Safe shutdown and non-volatile storage save */
    ECUM_STATE_SLEEP        /* Low power sleep mode */
} EcuM_StateType;

/**
 * @brief ECU Reset Request Types
 */
typedef enum {
    ECUM_RESET_NONE,
    ECUM_RESET_SOFT,        /* Triggered via UDS 0x11 service */
    ECUM_RESET_HARD         /* Triggered via hardware watchdog */
} EcuM_ResetType;

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: EcuM_Init
 * @brief Executes startup sequence: initializes MCAL drivers, NvM, BSW, and RTE.
 * @param: void
 * @return: void
 */
void EcuM_Init(void);

/**
 * Function name: EcuM_MainFunction
 * @brief Central state machine scheduler running main background loops.
 * @param: void
 * @return: void
 */
void EcuM_MainFunction(void);

/**
 * Function name: EcuM_SetResetRequest
 * @brief Sets the requested reset type flag (called by DcmSf for UDS 0x11).
 * @param: EcuM_ResetType reset_type
 * @return: void
 */
void EcuM_SetResetRequest(EcuM_ResetType reset_type);

/**
 * Function name: EcuM_GetState
 * @brief Returns the current operational state of the ECU.
 * @param: void
 * @return: EcuM_StateType
 */
EcuM_StateType EcuM_GetState(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* ECUM_H */