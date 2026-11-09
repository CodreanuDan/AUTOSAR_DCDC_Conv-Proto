/*
 * Filename: ParamHdl.h
 * @brief: 
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#ifndef PARAMHDL_H
#define PARAMHDL_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PARAMHDL_NRC_COND   0x22U   /* conditionsNotCorrect (read-only DID) */
#define PARAMHDL_NRC_OOR    0x31U   /* requestOutOfRange (value outside limits) */

#define PARAMHDL_SW_MAJOR   1U
#define PARAMHDL_SW_MINOR   0U
#define PARAMHDL_SW_PATCH   0U

/* Coding parameter ids = index in the coding block, also used as getter id */
typedef enum
{
    PARAM_SETPOINT_MIN_V = 0,   /* 0x0200 */
    PARAM_SETPOINT_MAX_V,       /* 0x0201 */
    PARAM_IIN_SHUTDOWN_A10,     /* 0x0202 0.1 A */
    PARAM_IOUT_DTC_A10,         /* 0x0203 0.1 A */
    PARAM_VIN_OV_DTC_V,         /* 0x0204 */
    PARAM_VIN_SHUTDOWN_V,       /* 0x0205 */
    PARAM_VIN_UV_DTC_V,         /* 0x0206 */
    PARAM_OVERTEMP_CRIT_C,      /* 0x0207 */
    PARAM_RECOVERY_MODE_DEF,    /* 0x0208 */
    PARAM_FAN_MODE_DEF,         /* 0x0209 */
    PARAM_RETRY_LIMIT,          /* 0x020A */
    PARAM_PWM_FREQ_HZ,          /* 0x020B 2 bytes, little endian */
    PARAM_COUNT
} ParamHdl_CodingIdType;

/* Loads both blocks (CRC check, defaults on failure), latches the ACTIVE coding
 * values and pushes the default modes to the RTE. Call after NvM_ReadAll(). */
void ParamHdl_Init(void);

/* DID services used by the DCM (the table lives only here) */
bool    ParamHdl_IsParamDid(uint16_t did, uint8_t *size, uint8_t *writable);
uint8_t ParamHdl_ReadDid (uint16_t did, uint8_t *out);        /* 0 = OK, else NRC */
uint8_t ParamHdl_WriteDid(uint16_t did, const uint8_t *in);   /* 0 = OK, else NRC */

/* Active coding value (latched at init, changes only after a reset) */
uint16_t ParamHdl_Get(ParamHdl_CodingIdType id);   

#ifdef __cplusplus
}
#endif
#endif
