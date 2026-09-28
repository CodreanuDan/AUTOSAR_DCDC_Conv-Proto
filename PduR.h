/*
 * Filename: PduR.h
 * @brief: Contains function declarations and PDU routing types for PDU Router module
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef PDUR_H
#define PDUR_H

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
 * Function name: PduR_ComRxIndication
 * @brief Receives a validated 12-byte PDU from ComSf, inspects its service identifier (SID),
 * and routes it to the appropriate BSW module (e.g., DcmSf for UDS requests).
 * @param: const uint8_t* pdu_ptr (Pointer to the received 12-byte data buffer)
 * @return: void
 */
void PduR_ComRxIndication(const uint8_t *pdu_ptr);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* PDUR_H */