/*
 * Filename: DemSf.h
 * @brief: Contains types, macro definitions, and function prototypes for Diagnostic Event Manager (DemSf)
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef DEM_SF_H
#define DEM_SF_H

/* Core libs */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Project specific libs */
#include "StdTypes.h"

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *            TYPE DEFINITIONS & MACROS
 *******************************************************/

/* Qualifying Status Types */
#define DEM_STATUS_OK          0U  
#define DEM_STATUS_PENDING     1U  
#define DEM_STATUS_CONFIRMED   2U

/* Fault Status Memory Flags */
#define DTC_STATUS_MISSING     0U
#define DTC_STATUS_PASSIVE     1U
#define DTC_STATUS_ACTIVE      2U

/* Debouncing Limits */
#define DEBOUNCE_THRESHOLD     100U

/* Electrical Fault Mode Types */
#define SHORT_TO_VBAT          1U
#define SHORT_TO_GND           2U
#define OPEN_CIRCUIT_VOLTAGE   3U
#define OPEN_CIRCUIT_CURRENT   4U

/**
 * @brief DTC Event Record Structure
 */
typedef struct {
    uint32_t           dtc_code;             /* 24-bit UDS DTC Identifier */
    uint8_t            qualifying_status;    /* DEM_STATUS_OK, PENDING, or CONFIRMED */
    uint8_t            fault_status;         /* DTC_STATUS_MISSING, PASSIVE, or ACTIVE */
    uint8_t            debounce_cnt;         /* Current debouncing counter value */
    uint16_t           freeze_frame_data;    /* Captured ADC value on fault confirmation */
    uint8_t            adc_source_ptr;       /* Pointer to hardware raw ADC channel */
    uint8_t          (*monitor_func)(uint8_t param, uint8_t mode); /* Monitor function pointer */
    uint8_t            error_mode;           /* Fault evaluation condition mode */
} DTC_StatusType;

/*******************************************************
 *            EXTERNAL GLOBAL VARIABLES
 *******************************************************/

extern DTC_StatusType g_dtc_table[];

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Dem_Init
 * @brief Initializes the Diagnostic Event Manager state machine and clears transient status counters.
 * @param: void
 * @return: void
 */
void Dem_Init(void);

/**
 * Function name: DemSf_MainFunction
 * @brief Cyclic task (e.g., 10ms) that evaluates fault monitoring functions and updates debouncing counters.
 * @param: void
 * @return: void
 */
void DemSf_MainFunction(void);

/**
 * Function name: DemSf_Monitor_ElectricalFaults
 * @brief Evaluates electrical raw ADC counts against defined threshold boundaries.
 * @param: uint8_t param (ADC Channel ID)
 * @param: uint8_t mode (Fault condition mode: SHORT_TO_GND, SHORT_TO_VBAT, OPEN_CIRCUIT)
 * @return: uint8_t (1 if fault condition is present, 0 otherwise)
 */
uint8_t DemSf_Monitor_ElectricalFaults(uint8_t param, uint8_t mode);

/**
 * Function name: Dem_ClearDiagnosticInformation
 * @brief Clears confirmed and passive DTCs from fault memory (Triggered by UDS Service 0x14).
 * @param: void
 * @return: void
 */
void Dem_ClearDiagnosticInformation(void);

/**
 * Function name: Dem_ReportDTCByStatusMask
 * @brief Triggers multi-frame active/passive DTC payload transmission via ComSf/DcmSf (Triggered by UDS Service 0x19).
 * @param: void
 * @return: void
 */
void Dem_ReportDTCByStatusMask(void);

/**
 * Function name: Dem_GetTotalDtcs
 * @brief Returns the dynamically calculated number of registered DTCs in the system.
 * @param: void
 * @return: uint8_t Count of DTCs in table
 */
uint8_t Dem_GetTotalDtcs(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* DEM_SF_H */