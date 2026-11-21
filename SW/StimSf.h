/*
 * Filename: StimSf.h
 * @brief: Tester-driven sensor override (stimulus). Replaces the RAW sensor values (ADC counts, SHT21 values,
 *         fuse sense level) so scaling, plausibility checks and DEM all run on the injected data.
 *         - enable mask   : DID 0x012C (extended session only), one bit per sensor
 *         - values        : stimulus frames 0xB0 / 0xB1 sent by the tester, no response
 *         - release       : no stimulus frame for 500 ms, mask written to 0, 10 01 or ECU reset
 *         Compile it out with DIAG_SENSOR_OVERRIDE = 0.
 */
#ifndef STIMSF_H
#define STIMSF_H

#include <stdint.h>
#include <stdbool.h>

#ifndef DIAG_SENSOR_OVERRIDE
#define DIAG_SENSOR_OVERRIDE  1
#endif

/* Override mask bits (DID 0x012C) */
#define STIM_BIT_VIN    0x01U
#define STIM_BIT_VOUT   0x02U
#define STIM_BIT_IIN    0x04U
#define STIM_BIT_IOUT   0x08U
#define STIM_BIT_TEMP   0x10U
#define STIM_BIT_HUM    0x20U
#define STIM_BIT_FUSE   0x40U
#define STIM_MASK_ALL   0x7FU

/* Stimulus frames (tester -> ECU, 12 bytes like every other frame, all values big-endian)
 *   0xB0: [1..2] Vin  [3..4] Vout  [5..6] Iin  [7..8] Iout   (10-bit ADC counts)
 *   0xB1: [1..2] Temp (int16, 1/100 degC)  [3..4] Hum (uint16, 1/100 %)  [5] Fuse sense level (1 = OK, 0 = blown)
 *   [9] free (tester sequence counter)
 */
#define STIM_FRAME_ADC   0xB0U
#define STIM_FRAME_MISC  0xB1U

#define STIM_TIMEOUT_TICKS  5U      /* 5 x 100 ms without a stimulus frame -> release */

#if DIAG_SENSOR_OVERRIDE

#ifdef __cplusplus
extern "C" {
#endif

/** Called every 100 ms: timeout supervision. */
void Stim_MainFunction(void);

/** Called by the PduR for 0xB0 / 0xB1 (ignored while no override is enabled). */
void Stim_RxIndication(const uint8_t *pdu_ptr);

/** DID 0x012C. Bits that go from 0 to 1 start at the current real value (bumpless). */
void    Stim_SetMask(uint8_t mask);
uint8_t Stim_GetMask(void);
void    Stim_Clear(void);

/** Hooks in IoHwAb: they get the real value and return the value to use. They also remember the real value. */
uint16_t Stim_ApplyAdc(uint8_t adc_channel, uint16_t real);       /* ADC_CH_IIN .. ADC_CH_VOUT (0..3) */
bool     Stim_IsAdcOverridden(uint8_t adc_channel);
int16_t  Stim_ApplyTemp(int16_t real);
uint16_t Stim_ApplyHum(uint16_t real);
uint8_t  Stim_ApplyFuse(uint8_t real);

#ifdef __cplusplus
}
#endif

#else   /* override compiled out: everything collapses to pass-through */

#define Stim_MainFunction()            ((void)0)
#define Stim_RxIndication(p)           ((void)(p))
#define Stim_SetMask(m)                ((void)(m))
#define Stim_GetMask()                 (0U)
#define Stim_Clear()                   ((void)0)
#define Stim_ApplyAdc(ch, real)        (real)
#define Stim_IsAdcOverridden(ch)       (false)
#define Stim_ApplyTemp(real)           (real)
#define Stim_ApplyHum(real)            (real)
#define Stim_ApplyFuse(real)           (real)

#endif

#endif /* STIMSF_H */
