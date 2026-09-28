/*
 * Filename: DcmSf.h
 * @brief: Contains layer definitions and function prototypes for DcmSf (DSL, DSD, DSP)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef DCM_SF_H
#define DCM_SF_H

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

/* UDS Service Identifiers (SID) */
#define UDS_SID_ECU_RESET               0x11U
#define UDS_SID_CLEAR_FAULT_MEM         0x14U
#define UDS_SID_READ_DTC_INFO           0x19U
#define UDS_SID_READ_DATA_BY_ID         0x22U
#define UDS_SID_WRITE_DATA_BY_ID        0x2EU
#define UDS_SID_ROUTINE_CONTROL         0x31U

/* Negative Response Codes (NRC) */
#define UDS_NRC_SERVICE_NOT_SUPPORTED   0x11U
#define UDS_NRC_SUB_FUNCTION_NOT_SUPP   0x12U
#define UDS_NRC_INCORRECT_LENGTH_OR_FORMAT 0x13U
#define UDS_NRC_CONDITIONS_NOT_CORRECT  0x22U
#define UDS_NRC_REQUEST_OUT_OF_RANGE    0x31U

/* Diagnostic Sessions (DSL - Diagnostic Session Layer) */
typedef enum
{
    DCM_DEFAULT_SESSION     = 0x01U,
    DCM_PROGRAMMING_SESSION = 0x02U,
    DCM_EXTENDED_SESSION    = 0x03U
} Dcm_SesCtrlType;

/************* TYPE DEFINITIONS & MACROS ************/


/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Dcm_Init
 * @brief Initializes the DCM module, resetting diagnostic session to DEFAULT.
 * @param: void
 * @return: void
 */
void Dcm_Init(void);

/**
 * Function name: Dcm_ProcessRxPdu
 * @brief Main entry point for diagnostic requests routed by PduR (DSD - Diagnostic Service Dispatcher).
 * @param: const uint8_t* pdu_ptr (Pointer to 12-byte UDS request payload)
 * @return: void
 */
void Dcm_ProcessRxPdu(const uint8_t *pdu_ptr);

/**
 * Function name: Dcm_GetSesCtrlType
 * @brief Returns the current active diagnostic session (DSL Layer).
 * @param: void
 * @return: Dcm_SesCtrlType (Current session)
 */
Dcm_SesCtrlType Dcm_GetSesCtrlType(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* DCM_SF_H */