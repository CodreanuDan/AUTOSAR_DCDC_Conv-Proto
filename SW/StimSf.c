/*
 * Filename: StimSf.c
 * @brief: Tester-driven sensor override, see StimSf.h
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */
#pragma GCC optimize ("Os")

#include "StimSf.h"

#if DIAG_SENSOR_OVERRIDE

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/
#define STIM_ADC_CHANNELS   4U
#define STIM_ADC_MAX        1023U

/* ADC channel index (ADC_CH_IIN 0, VIN 1, IOUT 2, VOUT 3) -> mask bit */
static const uint8_t s_adc_bit[STIM_ADC_CHANNELS] = { STIM_BIT_IIN, STIM_BIT_VIN, STIM_BIT_IOUT, STIM_BIT_VOUT };

static uint8_t  s_mask = 0U;
static uint8_t  s_age  = 0U;                       /* 100 ms ticks since the last stimulus frame */

static uint16_t s_adc_ovr[STIM_ADC_CHANNELS];      /* injected values */
static int16_t  s_temp_ovr;
static uint16_t s_hum_ovr;
static uint8_t  s_fuse_ovr = 1U;

static uint16_t s_adc_real[STIM_ADC_CHANNELS];     /* last real values, used for the bumpless start */
static int16_t  s_temp_real;
static uint16_t s_hum_real;
static uint8_t  s_fuse_real = 1U;

/********************************************************
 *             START OF FUNCTION DEFINITIONS
 ********************************************************/

static uint16_t Stim_U16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8U) | p[1]);
}

void Stim_SetMask(uint8_t mask)
{
    uint8_t i;
    uint8_t added;

    mask &= STIM_MASK_ALL;
    added = (uint8_t)(mask & (uint8_t)~s_mask);

    /* a sensor that is newly overridden keeps its current value until the first stimulus frame arrives */
    for (i = 0U; i < STIM_ADC_CHANNELS; i++)
    {
        if ((added & s_adc_bit[i]) != 0U) { s_adc_ovr[i] = s_adc_real[i]; }
    }
    if ((added & STIM_BIT_TEMP) != 0U) { s_temp_ovr = s_temp_real; }
    if ((added & STIM_BIT_HUM)  != 0U) { s_hum_ovr  = s_hum_real; }
    if ((added & STIM_BIT_FUSE) != 0U) { s_fuse_ovr  = s_fuse_real; }

    s_mask = mask;
    s_age  = 0U;                                    /* the tester has STIM_TIMEOUT_TICKS to start streaming */
}

uint8_t Stim_GetMask(void)
{
    return s_mask;
}

void Stim_Clear(void)
{
    s_mask = 0U;
    s_age  = 0U;
}

void Stim_MainFunction(void)
{
    if (s_mask == 0U)
    {
        return;
    }
    if (s_age < 0xFFU) { s_age++; }
    if (s_age >= STIM_TIMEOUT_TICKS)
    {
        Stim_Clear();                               /* tester gone: back to the real sensors */
    }
}

void Stim_RxIndication(const uint8_t *pdu_ptr)
{
    uint8_t i;
    uint16_t v;

    if ((pdu_ptr == 0) || (s_mask == 0U))
    {
        return;
    }

    if (pdu_ptr[0] == STIM_FRAME_ADC)
    {
        for (i = 0U; i < STIM_ADC_CHANNELS; i++)
        {
            /* frame order: Vin, Vout, Iin, Iout */
            static const uint8_t frame_ch[STIM_ADC_CHANNELS] = { 1U, 3U, 0U, 2U };   /* -> ADC channel index */
            uint8_t ch = frame_ch[i];
            if ((s_mask & s_adc_bit[ch]) != 0U)
            {
                v = Stim_U16(&pdu_ptr[1U + (2U * i)]);
                s_adc_ovr[ch] = (v > STIM_ADC_MAX) ? STIM_ADC_MAX : v;
            }
        }
        s_age = 0U;
    }
    else if (pdu_ptr[0] == STIM_FRAME_MISC)
    {
        if ((s_mask & STIM_BIT_TEMP) != 0U) { s_temp_ovr = (int16_t)Stim_U16(&pdu_ptr[1]); }
        if ((s_mask & STIM_BIT_HUM)  != 0U) { s_hum_ovr  = Stim_U16(&pdu_ptr[3]); }
        if ((s_mask & STIM_BIT_FUSE) != 0U) { s_fuse_ovr = (pdu_ptr[5] != 0U) ? 1U : 0U; }
        s_age = 0U;
    }
    else
    {
        /* not a stimulus frame */
    }
}

uint16_t Stim_ApplyAdc(uint8_t adc_channel, uint16_t real)
{
    if (adc_channel >= STIM_ADC_CHANNELS)
    {
        return real;
    }
    s_adc_real[adc_channel] = real;
    return ((s_mask & s_adc_bit[adc_channel]) != 0U) ? s_adc_ovr[adc_channel] : real;
}

bool Stim_IsAdcOverridden(uint8_t adc_channel)
{
    return (adc_channel < STIM_ADC_CHANNELS) && ((s_mask & s_adc_bit[adc_channel]) != 0U);
}

int16_t Stim_ApplyTemp(int16_t real)
{
    s_temp_real = real;
    return ((s_mask & STIM_BIT_TEMP) != 0U) ? s_temp_ovr : real;
}

uint16_t Stim_ApplyHum(uint16_t real)
{
    s_hum_real = real;
    return ((s_mask & STIM_BIT_HUM) != 0U) ? s_hum_ovr : real;
}

uint8_t Stim_ApplyFuse(uint8_t real)
{
    s_fuse_real = real;
    return ((s_mask & STIM_BIT_FUSE) != 0U) ? s_fuse_ovr : real;
}

#endif /* DIAG_SENSOR_OVERRIDE */

/* End of StimSf.c */
