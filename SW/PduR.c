/*
 * Filename: PduR.c
 * @brief: Contains routing logic definitions for PDU Router module
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */
#pragma GCC optimize ("Os")

#include <stddef.h>
#include "PduR.h"
#include "DcmSf.h"
#include "StimSf.h"

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: PduR_ComRxIndication
 * @brief Receives a validated 12-byte PDU from ComSf, inspects its service identifier (SID),
 * and routes it to the appropriate BSW module (e.g., DcmSf for UDS requests).
 * @param: const uint8_t* pdu_ptr (Pointer to the received 12-byte data buffer)
 * @return: void
 */
void PduR_ComRxIndication(const uint8_t *pdu_ptr)
{
    if (pdu_ptr != NULL)
    {
        uint8_t sid = pdu_ptr[0];

        /* Inspect Service Identifier (SID) to route diagnostic frames */
        switch (sid)
        {
			case 0x10:  /* Diagnostic Session Control */
            case 0x11U: /* ECU Reset */
            case 0x14U: /* Clear Diagnostic Information */
            case 0x19U: /* Read DTC Information */
            case 0x22U: /* Read Data By Identifier */
            case 0x2EU: /* Write Data By Identifier */
            case 0x31U: /* Routine Control */
                /* Route payload to Diagnostic Communication Manager */
                Dcm_ProcessRxPdu(pdu_ptr);
                break;
		    case STIM_FRAME_ADC:    /* 0xB0 sensor override values (no response) */
            case STIM_FRAME_MISC:   /* 0xB1 sensor override values (no response) */
                Stim_RxIndication(pdu_ptr);
                break;
            default:
                /* Unknown/Unsupported PDU SID - Ignore frame */
                break;
        }
    }
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of PduR.c */