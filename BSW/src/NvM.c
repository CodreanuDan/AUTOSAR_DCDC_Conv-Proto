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
#include "Rte.h"

/*******************************************************
 *            OFFSETS & CONSTANTS IN EEPROM
 *******************************************************/
#define NVM_ADDR_MAGIC         ((uint16_t*)0x0000U)
#define NVM_ADDR_TARGET_FREQ   ((uint16_t*)0x0002U)
#define NVM_ADDR_TARGET_VOUT   ((uint8_t*) 0x0004U)
#define NVM_ADDR_PID_DISABLE   ((uint8_t*) 0x0005U)
#define NVM_ADDR_DTC_STATUSES  ((uint8_t*) 0x0006U)

#define NVM_MAGIC_VALUE        0xA55AU

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

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
    uint16_t magic = eeprom_read_word(NVM_ADDR_MAGIC);
    if (magic != NVM_MAGIC_VALUE)
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
	uint8_t i;

	/* Read persistent parameters from EEPROM and update RTE buffers */
	uint16_t freq = eeprom_read_word(NVM_ADDR_TARGET_FREQ);
    uint8_t vout  = eeprom_read_byte(NVM_ADDR_TARGET_VOUT);
    bool pid_dis  = (bool)eeprom_read_byte(NVM_ADDR_PID_DISABLE);

	Rte_Write_TargetFrequency(freq);
    Rte_Write_PidTargetSetpoint(vout);
    Rte_Write_PidDisableFlag(pid_dis);

	/* Sanity checks for blank EEPROM */
    if (freq == 0xFFFFU) { freq = 100U; }
    if (vout == 0xFFU)   { vout = 12U;  }

    /* Restore DTC fault statuses */
    uint8_t total_dtcs = Dem_GetTotalDtcs();
    for (i = 0U; i < total_dtcs; i++)
    {
        uint8_t status = eeprom_read_byte(NVM_ADDR_DTC_STATUSES + i);
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
	uint8_t i;

	/* Write magic header */
    eeprom_update_word(NVM_ADDR_MAGIC, NVM_MAGIC_VALUE);

    /* Fetch current values from RTE and update EEPROM */
    eeprom_update_word(NVM_ADDR_TARGET_FREQ, Rte_Read_TargetFrequency());
    eeprom_update_byte(NVM_ADDR_TARGET_VOUT, Rte_Read_PidTargetSetpoint());
    eeprom_update_byte(NVM_ADDR_PID_DISABLE, Rte_Read_PidDisableFlag() ? 1U : 0U);

    /* Save DTC fault statuses */
    uint8_t total_dtcs = Dem_GetTotalDtcs();
    for (i = 0U; i < total_dtcs; i++)
    {
		eeprom_update_byte(NVM_ADDR_DTC_STATUSES + i, g_dtc_table[i].fault_status);
    }
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of NvM.c */