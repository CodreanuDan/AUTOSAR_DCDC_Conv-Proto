/*
 * Filename: NvM.c
 * @brief: Contains EEPROM storage mapping and function definitions for NvM
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

/* Core libs */
#include <avr/eeprom.h>

/* Project specific libs */
#include "NvM.h"
#include "DemSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/* Persistent storage structure mirrored in internal EEPROM */
typedef struct {
    uint16_t calibration_magic;
    uint16_t target_frequency;
    uint8_t  target_vout;
    uint8_t  pid_disable_flag;
    uint8_t  dtc_fault_statuses[12]; /* Mirror of fault memory statuses */
} NvM_BlockConfigType;

/* EEPROM allocated storage block */
EEMEM NvM_BlockConfigType EEP_BlockConfig = {
    .calibration_magic = 0xA55AU,
    .target_frequency  = 100U,
    .target_vout       = 12U,
    .pid_disable_flag  = 1U,
    .dtc_fault_statuses = {0}
};

/* External configuration variables from application */
extern uint16_t g_target_frequency;
extern uint8_t  g_target_vout;
extern uint8_t  g_pid_disable_flag;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: NvM_Init
 * @brief Initializes the Non-Volatile Memory manager and verifies EEPROM block integrity.
 * @param: void
 * @return: void
 */
void NvM_Init(void)
{
    /* Read magic header from EEPROM to verify first-time initialization */
    uint16_t magic = eeprom_read_word(&(EEP_BlockConfig.calibration_magic));
    if (magic != 0xA55AU)
    {
        /* First boot or corrupted EEPROM: Write default baseline blocks */
        NvM_WriteAll();
    }
}

/**
 * Function name: NvM_ReadAll
 * @brief Reads persistent calibration data, configurations, and DTC status from EEPROM into RAM.
 * @param: void
 * @return: void
 */
void NvM_ReadAll(void)
{
    g_target_frequency   = eeprom_read_word(&(EEP_BlockConfig.target_frequency));
    g_target_vout        = eeprom_read_byte(&(EEP_BlockConfig.target_vout));
    g_pid_disable_flag   = eeprom_read_byte(&(EEP_BlockConfig.pid_disable_flag));

    /* Restore DTC fault statuses */
    uint8_t total_dtcs = Dem_GetTotalDtcs();
    for (uint8_t i = 0U; i < total_dtcs; i++)
    {
        uint8_t status = eeprom_read_byte(&(EEP_BlockConfig.dtc_fault_statuses[i]));
        g_dtc_table[i].fault_status = status;
        if (status != DTC_STATUS_MISSING)
        {
            g_dtc_table[i].qualifying_status = DEM_STATUS_CONFIRMED;
        }
    }
}

/**
 * Function name: NvM_WriteAll
 * @brief Writes current runtime calibration data, configuration parameters, and DTC history to EEPROM.
 * @param: void
 * @return: void
 */
void NvM_WriteAll(void)
{
    eeprom_update_word(&(EEP_BlockConfig.calibration_magic), 0xA55AU);
    eeprom_update_word(&(EEP_BlockConfig.target_frequency), g_target_frequency);
    eeprom_update_byte(&(EEP_BlockConfig.target_vout), g_target_vout);
    eeprom_update_byte(&(EEP_BlockConfig.pid_disable_flag), g_pid_disable_flag);

    /* Save DTC fault statuses */
    uint8_t total_dtcs = Dem_GetTotalDtcs();
    for (uint8_t i = 0U; i < total_dtcs; i++)
    {
        eeprom_update_byte(&(EEP_BlockConfig.dtc_fault_statuses[i]), g_dtc_table[i].fault_status);
    }
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of NvM.c */