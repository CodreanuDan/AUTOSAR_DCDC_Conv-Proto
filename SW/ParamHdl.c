/*
 * Filename: ParamHdl.c
 * @brief: 
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include <string.h>
#include <avr/pgmspace.h>
#include "ParamHdl.h"
#include "NvM.h"
#include "Rte.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

#define PH_KIND_CONST   0U   /* compiled in, read-only          */
#define PH_KIND_ASCII   1U   /* 0x20..0x7E per byte             */
#define PH_KIND_RAW     2U   /* any bytes                       */
#define PH_KIND_U8      3U   /* 1 byte with min / max           */
#define PH_KIND_U16     4U   /* 2 bytes little endian, min / max */
#define PH_KIND_STATUS  5U   /* runtime status, read-only       */

#define PH_BLK_ID       0U   /* = NVM_BLK_IDENT  */
#define PH_BLK_CODING   1U   /* = NVM_BLK_CODING */

typedef struct {
    uint16_t    did;
    uint8_t     block;
    uint8_t     offset;     /* inside the block data area */
    uint8_t     size;
    uint8_t     kind;
    uint8_t     id;         /* ParamHdl_CodingIdType for coding entries */
    uint16_t    min;
    uint16_t    max;
    uint16_t    def;        /* default for U8 / U16 */
    const char *def_str;    /* default for ASCII / RAW */
} Ph_DidCfg;

/* Moved to PROGMEM t0 save RAM space */
static const char PS_TEIL[]  PROGMEM = "DCDC001";
static const char PS_HWNR[]  PROGMEM = "DCDCHW1";
static const char PS_HWVER[] PROGMEM = "\x01";
static const char PS_SERIAL[] PROGMEM = "SN00000";
static const char PS_VIN1[]  PROGMEM = "DCDC000";
static const char PS_VIN2[]  PROGMEM = "0000000";
static const char PS_VIN3[]  PROGMEM = "001";

static const Ph_DidCfg k_did[] =
{
    /* ---- Identification ---- */
    { 0xF187U, PH_BLK_ID,  0U, 7U, PH_KIND_ASCII, 0U, 0U, 0U, 0U, PS_TEIL   },  /* Teilenummer */
    { 0xF191U, PH_BLK_ID,  7U, 7U, PH_KIND_ASCII, 0U, 0U, 0U, 0U, PS_HWNR   },  /* HW number   */
    { 0xF193U, PH_BLK_ID, 14U, 2U, PH_KIND_RAW,   0U, 0U, 0U, 0U, PS_HWVER  },  /* HW version  */
    { 0xF18CU, PH_BLK_ID, 16U, 7U, PH_KIND_ASCII, 0U, 0U, 0U, 0U, PS_SERIAL },  /* Serial      */
    { 0xF190U, PH_BLK_ID, 23U, 7U, PH_KIND_ASCII, 0U, 0U, 0U, 0U, PS_VIN1   },  /* pseudo-VIN 1..7   */
    { 0xF1A1U, PH_BLK_ID, 30U, 7U, PH_KIND_ASCII, 0U, 0U, 0U, 0U, PS_VIN2   },  /* pseudo-VIN 8..14  */
    { 0xF1A2U, PH_BLK_ID, 37U, 3U, PH_KIND_ASCII, 0U, 0U, 0U, 0U, PS_VIN3   },  /* pseudo-VIN 15..17 */
    { 0xF189U, 0U, 0U, 3U, PH_KIND_CONST,  0U, 0U, 0U, 0U, 0 },                 /* SW version  */
    { 0xF1F0U, 0U, 0U, 1U, PH_KIND_STATUS, 0U, 0U, 0U, 0U, 0 },                 /* status: bit0 ident defaults, bit1 coding defaults */

    /* ---- Variant coding ---- */
    { 0x0200U, PH_BLK_CODING,  0U, 1U, PH_KIND_U8,  PARAM_SETPOINT_MIN_V,    1U,  30U,   5U, 0 },
    { 0x0201U, PH_BLK_CODING,  1U, 1U, PH_KIND_U8,  PARAM_SETPOINT_MAX_V,    5U,  35U,  24U, 0 },
    { 0x0202U, PH_BLK_CODING,  2U, 1U, PH_KIND_U8,  PARAM_IIN_SHUTDOWN_A10, 20U, 100U,  80U, 0 },
    { 0x0203U, PH_BLK_CODING,  3U, 1U, PH_KIND_U8,  PARAM_IOUT_DTC_A10,     20U, 100U,  80U, 0 },
    { 0x0204U, PH_BLK_CODING,  4U, 1U, PH_KIND_U8,  PARAM_VIN_OV_DTC_V,     12U,  20U,  16U, 0 },
    { 0x0205U, PH_BLK_CODING,  5U, 1U, PH_KIND_U8,  PARAM_VIN_SHUTDOWN_V,   18U,  30U,  24U, 0 },
    { 0x0206U, PH_BLK_CODING,  6U, 1U, PH_KIND_U8,  PARAM_VIN_UV_DTC_V,      5U,  12U,   9U, 0 },
    { 0x0207U, PH_BLK_CODING,  7U, 1U, PH_KIND_U8,  PARAM_OVERTEMP_CRIT_C,  60U, 125U, 100U, 0 },
    { 0x0208U, PH_BLK_CODING,  8U, 1U, PH_KIND_U8,  PARAM_RECOVERY_MODE_DEF, 0U,   1U,   0U, 0 },
    { 0x0209U, PH_BLK_CODING,  9U, 1U, PH_KIND_U8,  PARAM_FAN_MODE_DEF,      0U,   1U,   0U, 0 },
    { 0x020AU, PH_BLK_CODING, 10U, 1U, PH_KIND_U8,  PARAM_RETRY_LIMIT,       1U,  10U,   3U, 0 },
    { 0x020BU, PH_BLK_CODING, 11U, 2U, PH_KIND_U16, PARAM_PWM_FREQ_HZ,     100U, 1000U, 250U, 0 }
};
#define PH_NUM_DIDS  (sizeof(k_did) / sizeof(k_did[0]))

static uint8_t  s_img[2][NVM_LEN_IDENT];      /* data images, ident is the larger block (40 B) */
static uint16_t s_active[PARAM_COUNT];        /* latched at init, changes only after a reset */
static uint8_t  s_status = 0U;                /* bit0 ident on defaults, bit1 coding on defaults */

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

static uint8_t Ph_Find(uint16_t did)
{
    uint8_t i;
    for (i = 0U; i < PH_NUM_DIDS; i++) 
	{ 
		if (k_did[i].did == did) 
		{ 
			return i; 
		} 
	}
    return 0xFFU;
}

static uint8_t Ph_IsNumeric(uint8_t kind) 
{ 
	return ((kind == PH_KIND_U8) || (kind == PH_KIND_U16)) ? 1U : 0U; 
}

static uint16_t Ph_ImgValue(const Ph_DidCfg *c)
{
    uint16_t v = s_img[c->block][c->offset];
    if (c->size == 2U) 
	{ 
		v |= (uint16_t)((uint16_t)s_img[c->block][c->offset + 1U] << 8U); 
	}
    return v;
}

static void Ph_LoadDefaults(uint8_t b)
{
    uint8_t i, j, len;
    memset(s_img[b], 0, NVM_LEN_IDENT);
    for (i = 0U; i < PH_NUM_DIDS; i++)
    {
        if ((Ph_Get(i)->block != b) || (Ph_Get(i)->kind == PH_KIND_CONST) || (Ph_Get(i)->kind == PH_KIND_STATUS)) 
		{ 
			continue; 
		}

        if (Ph_IsNumeric(Ph_Get(i)->kind) != 0U)
        {
            s_img[b][Ph_Get(i)->offset] = (uint8_t)(Ph_Get(i)->def & 0xFFU);
            if (Ph_Get(i)->size == 2U) 
			{ 
				s_img[b][Ph_Get(i)->offset + 1U] = (uint8_t)(Ph_Get(i)->def >> 8U); 
			}
        }
        else
        {
            len = (uint8_t)strlen(Ph_Get(i)->def_str);
            for (j = 0U; j < Ph_Get(i)->size; j++)
            {
                s_img[b][Ph_Get(i)->offset + j] = (j < len) ? (uint8_t)Ph_Get(i)->def_str[j] : ((Ph_Get(i)->kind == PH_KIND_ASCII) ? (uint8_t)' ' : 0U);
            }
        }
    }
}

static const Ph_DidCfg *Ph_Get(uint8_t i)
{
    static Ph_DidCfg s_cfg;                       /* one 16 B RAM scratch */
    memcpy_P(&s_cfg, Ph_Get(i), sizeof(Ph_DidCfg));
    return &s_cfg;
}

void ParamHdl_Init(void)
{
    uint8_t b, i;

    s_status = 0U;
    for (b = 0U; b < 2U; b++)
    {
        if (NvM_ReadBlock((NvM_BlockIdType)b, s_img[b]) != NVM_E_OK)
        {
            Ph_LoadDefaults(b);
            NvM_WriteBlock((NvM_BlockIdType)b, s_img[b]);
            s_status |= (uint8_t)(1U << b);
        }
    }

    for (i = 0U; i < PH_NUM_DIDS; i++)
    {
        if ((Ph_Get(i)->block == PH_BLK_CODING) && (Ph_IsNumeric(Ph_Get(i)->kind) != 0U))
        {
            s_active[Ph_Get(i)->id] = Ph_ImgValue(Ph_Get(i));
        }
    }

    for (i = 0U; i < (uint8_t)PH_NUM_DIDS; i++) 
	{ 
		Rte_Write_CodingParam(i, s_active[i]); 
	}

    Rte_Write_RecoveryMode((uint8_t)s_active[PARAM_RECOVERY_MODE_DEF]);
    Rte_Write_FanMode((uint8_t)s_active[PARAM_FAN_MODE_DEF]);
    Rte_Write_TargetFrequency(s_active[PARAM_PWM_FREQ_HZ]);
}

bool ParamHdl_IsParamDid(uint16_t did, uint8_t *size, uint8_t *writable)
{
    uint8_t i = Ph_Find(did);
    if (i == 0xFFU) 
	{ 
		return false; 
	}
    *size     = Ph_Get(i)->size;
    *writable = (uint8_t)(((Ph_Get(i)->kind == PH_KIND_CONST) || (Ph_Get(i)->kind == PH_KIND_STATUS)) ? 0U : 1U);
    return true;
}

uint8_t ParamHdl_ReadDid(uint16_t did, uint8_t *out)
{
    uint8_t i = Ph_Find(did), j;
    if (i == 0xFFU) 
	{ 
		return PARAMHDL_NRC_OOR; 
	}

    if (Ph_Get(i)->kind == PH_KIND_CONST)
    {
        out[0] = PARAMHDL_SW_MAJOR; out[1] = PARAMHDL_SW_MINOR; out[2] = PARAMHDL_SW_PATCH;
    }
    else if (Ph_Get(i)->kind == PH_KIND_STATUS)
    {
        out[0] = s_status;
    }
    else
    {
        for (j = 0U; j < Ph_Get(i)->size; j++) 
		{ 
			out[j] = s_img[Ph_Get(i)->block][Ph_Get(i)->offset + j]; 
		}
    }
    return 0U;
}

uint8_t ParamHdl_WriteDid(uint16_t did, const uint8_t *in)
{
    uint8_t i = Ph_Find(did), j;
    uint16_t v;
    if (i == 0xFFU) 
	{ 
		return PARAMHDL_NRC_OOR; 
	}
    if ((Ph_Get(i)->kind == PH_KIND_CONST) || (Ph_Get(i)->kind == PH_KIND_STATUS)) 
	{ 
		return PARAMHDL_NRC_COND; 
	}

    if (Ph_IsNumeric(Ph_Get(i)->kind) != 0U)
    {
        v = in[0];
        if (Ph_Get(i)->size == 2U) 
		{ 
			v |= (uint16_t)((uint16_t)in[1] << 8U); 
		}
        if ((v < Ph_Get(i)->min) || (v > Ph_Get(i)->max)) 
		{ 
			return PARAMHDL_NRC_OOR; 
		}
    }
    else if (Ph_Get(i)->kind == PH_KIND_ASCII)
    {
        for (j = 0U; j < Ph_Get(i)->size; j++) 
		{ 
			if ((in[j] < 0x20U) || (in[j] > 0x7EU)) 
			{ 
				return PARAMHDL_NRC_OOR; 
			} 
		}
    }

    for (j = 0U; j < Ph_Get(i)->size; j++) 
	{ 
		s_img[Ph_Get(i)->block][Ph_Get(i)->offset + j] = in[j]; 
	}

    NvM_WriteBlock((NvM_BlockIdType)Ph_Get(i)->block, s_img[Ph_Get(i)->block]);   /* stored now, active after reset */
    return 0U;
}

uint16_t ParamHdl_Get(ParamHdl_CodingIdType id) 
{ 
	return s_active[id]; 
}