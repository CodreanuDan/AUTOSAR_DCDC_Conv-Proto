/*
 * Filename: NvM.c
 * @brief: Contains EEPROM storage mapping and function definitions for NvM
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

/* Core libs */
#include <avr/eeprom.h>
#include <stddef.h>

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

#define NVM_MAGIC_VALUE        0xA55BU

/*******************************************************
 *         START OF VARIABLE AND DATA DEFINITIONS
 *******************************************************/
typedef struct {
    uint16_t addr;
    uint8_t  len;
    uint8_t  ver;    /* layout version: bump when the block layout changes */
} NvM_BlockCfg;

/* EEPROM: [ver][data ... len][crc8]. 0x0000..0x003F is reserved for the legacy area (magic, setpoint, DTCs) */
static const NvM_BlockCfg k_nvm_blk[NVM_NUM_BLOCKS] =
{
    { 0x0040U, NVM_LEN_IDENT,  0x01U },
    { 0x0080U, NVM_LEN_CODING, 0x01U }
};


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
        eeprom_update_word(NVM_ADDR_MAGIC, NVM_MAGIC_VALUE);
        eeprom_update_byte(NVM_ADDR_TARGET_VOUT, 12U);
        eeprom_update_byte(NVM_ADDR_PID_DISABLE, 0U);

        uint8_t total_dtcs = Dem_GetTotalDtcs();
		uint8_t i;
        for (i = 0U; i < total_dtcs; i++)
        {
            eeprom_update_byte(NVM_ADDR_DTC_STATUSES + i, DTC_STATUS_MISSING);
        }
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
    uint8_t vout  = eeprom_read_byte(NVM_ADDR_TARGET_VOUT);
    bool pid_dis  = (bool)eeprom_read_byte(NVM_ADDR_PID_DISABLE);

	/* Sanity checks for blank EEPROM */
    if (vout == 0xFFU)   { vout = 12U;  }

    Rte_Write_PidTargetSetpoint(vout);
    Rte_Write_PidDisableFlag(pid_dis);

    /* Restore DTC fault statuses */
    uint8_t total_dtcs = Dem_GetTotalDtcs();
    for (i = 0U; i < total_dtcs; i++)
    {
        uint8_t status = eeprom_read_byte(NVM_ADDR_DTC_STATUSES + i);
		if (status > DTC_STATUS_ACTIVE) { status = DTC_STATUS_MISSING; }   /* blank/stale EEPROM (0xFF) */
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
    eeprom_update_byte(NVM_ADDR_TARGET_VOUT, Rte_Read_PidTargetSetpoint());
    eeprom_update_byte(NVM_ADDR_PID_DISABLE, Rte_Read_PidDisableFlag() ? 1U : 0U);

    /* Save DTC fault statuses */
    uint8_t total_dtcs = Dem_GetTotalDtcs();
    for (i = 0U; i < total_dtcs; i++)
    {
		eeprom_update_byte(NVM_ADDR_DTC_STATUSES + i, g_dtc_table[i].fault_status);
    }
}

static uint8_t NvM_Crc8(uint8_t crc, const uint8_t *p, uint8_t n)
{
    uint8_t i, b;
    for (i = 0U; i < n; i++)
    {
        crc ^= p[i];
        for (b = 0U; b < 8U; b++)
        {
            crc = (uint8_t)((crc & 0x80U) ? ((crc << 1U) ^ 0x31U) : (crc << 1U));
        }
    }
    return crc;
}

uint8_t NvM_ReadBlock(NvM_BlockIdType id, uint8_t *data)
{
    uint8_t ver, crc_stored, crc;
    uint16_t addr = k_nvm_blk[id].addr;
    uint8_t  len  = k_nvm_blk[id].len;

    ver = eeprom_read_byte((const uint8_t*)addr);
    eeprom_read_block(data, (const void*)(addr + 1U), (size_t)len);
    crc_stored = eeprom_read_byte((const uint8_t*)(addr + 1U + len));

    crc = NvM_Crc8(0xFFU, &ver, 1U);
    crc = NvM_Crc8(crc, data, len);

    return ((ver == k_nvm_blk[id].ver) && (crc == crc_stored)) ? NVM_E_OK : NVM_E_INVALID;
}

void NvM_WriteBlock(NvM_BlockIdType id, const uint8_t *data)
{
    uint8_t ver = k_nvm_blk[id].ver;
    uint8_t crc;
    uint16_t addr = k_nvm_blk[id].addr;
    uint8_t  len  = k_nvm_blk[id].len;

    crc = NvM_Crc8(0xFFU, &ver, 1U);
    crc = NvM_Crc8(crc, data, len);

    eeprom_update_byte((uint8_t*)addr, ver);
    eeprom_update_block(data, (void*)(addr + 1U), (size_t)len);
    eeprom_update_byte((uint8_t*)(addr + 1U + len), crc);
}

/************* END OF FUNCTION DEFINITIONS ************/

/* End of NvM.c */