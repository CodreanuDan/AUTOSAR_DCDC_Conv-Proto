/******************************************************************************
 * @file    Sht21Sf.c
 * @brief   SHT21 driver - implementation
 *
 *  Measurement cycle (about 130 ms, then it starts over):
 *      TRIG_T  -> send command 0xF3 (temperature, "no hold master")
 *      CONV_T  -> wait >= 90 ms (conversion takes up to 85 ms)
 *      READ_T  -> read 3 bytes: MSB, LSB, CRC
 *      TRIG_RH -> send command 0xF5 (humidity, "no hold master")
 *      CONV_RH -> wait >= 30 ms (conversion takes up to 29 ms)
 *      READ_RH -> read 3 bytes
 *      -> convert both values, publish, restart
 *
 *  "No hold" means the sensor does not stretch SCL while measuring; the bus
 *  stays free and we simply wait with a timer. If we read too early the
 *  sensor NACKs its address (that is also how we notice problems).
 *
 *  Error handling: any NACK / timeout / CRC error counts as a failure and
 *  puts the driver into a 100 ms BACKOFF. After SHT21_FAIL_LIMIT consecutive
 *  failures the data is declared INVALID (the cooling logic will then use
 *  the fail-safe fan speed). The first fully good cycle makes it valid again.
 ******************************************************************************/
#include <util/atomic.h>
#include "StdTypes.h"
#include "I2cSf.h"
#include "GptSf.h"             /* extern volatile uint32_t g_tick_ms */
#include "Sht21Sf.h"

/* ---- Sensor constants ---------------------------------------------------- */
#define SHT21_ADDR              0x40U   /* fixed 7-bit address                */
#define SHT21_CMD_T_NOHOLD      0xF3U   /* trigger temperature, no hold       */
#define SHT21_CMD_RH_NOHOLD     0xF5U   /* trigger humidity, no hold          */

/* ---- Timing (ms) --------------------------------------------------------- */
#define SHT21_T_CONV_MS         90U     /* T conversion max 85 ms + margin    */
#define SHT21_RH_CONV_MS        30U     /* RH conversion max 29 ms + margin   */
#define SHT21_XFER_TIMEOUT_MS   20U     /* a 1..3 byte transfer at 100 kHz
                                           needs < 1 ms; 20 ms = hung bus     */
#define SHT21_BACKOFF_MS        100U    /* pause after an error before retry  */

/* ---- Robustness ---------------------------------------------------------- */
#define SHT21_FAIL_LIMIT        3U      /* consecutive failures -> INVALID    */
#define SHT21_CHECK_CRC         1U      /* 1 = verify CRC-8 of every reading  */

/* ---- State machine ------------------------------------------------------- */
typedef enum
{
    S_TRIG_T = 0,      /* start sending temperature command                   */
    S_WAIT_TX_T,       /* waiting for the I2C write to finish                 */
    S_CONV_T,          /* sensor is measuring temperature                     */
    S_READ_T,          /* start reading 3 bytes                               */
    S_WAIT_RX_T,       /* waiting for the I2C read to finish                  */
    S_TRIG_RH,         /* same sequence for humidity                          */
    S_WAIT_TX_RH,
    S_CONV_RH,
    S_READ_RH,
    S_WAIT_RX_RH,
    S_BACKOFF          /* error recovery pause                                */
} Sht21_StateType;

static Sht21_StateType s_state = S_TRIG_T;
static uint32_t        s_t0;                 /* timestamp: start of transfer
                                                or of the conversion wait     */
static uint8_t         s_cmd;                /* command byte (must live in
                                                RAM until the write ends!)    */
static uint8_t         s_rx[3];              /* receive buffer: MSB,LSB,CRC   */
static uint16_t        s_raw_t;              /* raw temperature (kept while
                                                the RH measurement runs)      */
static int16_t         s_temp_c100;          /* published temperature         */
static uint16_t        s_rh_c100;            /* published humidity            */
static uint8_t         s_valid = FALSE;      /* published validity            */
static uint8_t         s_fail_cnt;           /* consecutive failure counter   */

/* Atomic read of the 32-bit millisecond counter (ISR updates it; an AVR
 * needs several instructions to read 4 bytes -> could tear without this).   */
static uint32_t Tick(void)
{
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        t = g_tick_ms;
    }
    return t;
}

/* CRC-8 as specified by Sensirion: polynomial x^8+x^5+x^4+1 (0x31), init 0 */
static uint8_t Crc8(const uint8_t *d, uint8_t len)
{
    uint8_t crc = 0U;
    uint8_t i, b;
    for (i = 0U; i < len; i++)
    {
        crc ^= d[i];
        for (b = 0U; b < 8U; b++)
        {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U)
                                : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* Register a failure: count it, invalidate after the limit, go to BACKOFF. */
static void Fail(uint32_t now)
{
    if (s_fail_cnt < SHT21_FAIL_LIMIT)
    {
        s_fail_cnt++;
    }
    if (s_fail_cnt >= SHT21_FAIL_LIMIT)
    {
        s_valid = FALSE;       /* published data no longer trustworthy       */
    }
    s_t0    = now;
    s_state = S_BACKOFF;
}

/* Poll the running I2C transfer.
 * Return: 0 = still running, 1 = finished OK, 2 = failed (NACK/bus/timeout) */
static uint8_t Xfer_Poll(uint32_t now)
{
    switch (I2c_GetStatus())
    {
        case I2C_OK:
            return 1U;

        case I2C_ERR_NACK:
        case I2C_ERR_BUS:
            return 2U;

        default:               /* still BUSY: watch for a hung bus            */
            if ((uint32_t)(now - s_t0) > SHT21_XFER_TIMEOUT_MS)
            {
                I2c_Abort();   /* release the bus so the next try can work    */
                return 2U;
            }
            return 0U;
    }
}

/* Validate the 3 received bytes and extract the 14/12-bit raw value.
 * The 2 lowest bits of the 16-bit word are status bits -> mask them (0xFFFC).
 * Return TRUE if the CRC is good (or CRC checking is disabled).             */
static uint8_t Parse(uint16_t *raw)
{
#if (SHT21_CHECK_CRC == 1U)
    if (Crc8(s_rx, 2U) != s_rx[2])
    {
        return FALSE;
    }
#endif
    *raw = (uint16_t)((((uint16_t)s_rx[0] << 8) | s_rx[1]) & 0xFFFCU);
    return TRUE;
}

void Sht21_Init(void)
{
    s_state     = S_TRIG_T;
    s_valid     = FALSE;       /* invalid until the first full good cycle     */
    s_fail_cnt  = 0U;
    s_temp_c100 = 0;
    s_rh_c100   = 0U;
}

void Sht21_MainFunction(void)
{
    uint32_t now = Tick();
    uint16_t raw_rh;
    uint8_t  r;
    int32_t  rh, t;

    switch (s_state)
    {
        /* ======================= TEMPERATURE ============================== */
        case S_TRIG_T:
            s_cmd = SHT21_CMD_T_NOHOLD;
            if (I2c_StartWrite(SHT21_ADDR, &s_cmd, 1U) != 0U)
            {
                s_t0    = now;          /* for the transfer timeout           */
                s_state = S_WAIT_TX_T;
            }
            /* else: driver busy -> simply try again at the next call         */
            break;

        case S_WAIT_TX_T:
            r = Xfer_Poll(now);
            if (r == 1U)
            {
                s_t0    = now;          /* start of conversion wait           */
                s_state = S_CONV_T;
            }
            else if (r == 2U)
            {
                Fail(now);
            }
            break;

        case S_CONV_T:
            if ((uint32_t)(now - s_t0) >= SHT21_T_CONV_MS)
            {
                s_state = S_READ_T;
            }
            break;

        case S_READ_T:
            if (I2c_StartRead(SHT21_ADDR, s_rx, 3U) != 0U)
            {
                s_t0    = now;
                s_state = S_WAIT_RX_T;
            }
            break;

        case S_WAIT_RX_T:
            r = Xfer_Poll(now);
            if (r == 1U)
            {
                if (Parse(&s_raw_t) != FALSE) { s_state = S_TRIG_RH; }
                else                          { Fail(now); }   /* CRC error   */
            }
            else if (r == 2U)
            {
                Fail(now);
            }
            break;

        /* ========================== HUMIDITY ============================== */
        case S_TRIG_RH:
            s_cmd = SHT21_CMD_RH_NOHOLD;
            if (I2c_StartWrite(SHT21_ADDR, &s_cmd, 1U) != 0U)
            {
                s_t0    = now;
                s_state = S_WAIT_TX_RH;
            }
            break;

        case S_WAIT_TX_RH:
            r = Xfer_Poll(now);
            if (r == 1U)      { s_t0 = now; s_state = S_CONV_RH; }
            else if (r == 2U) { Fail(now); }
            break;

        case S_CONV_RH:
            if ((uint32_t)(now - s_t0) >= SHT21_RH_CONV_MS)
            {
                s_state = S_READ_RH;
            }
            break;

        case S_READ_RH:
            if (I2c_StartRead(SHT21_ADDR, s_rx, 3U) != 0U)
            {
                s_t0    = now;
                s_state = S_WAIT_RX_RH;
            }
            break;

        case S_WAIT_RX_RH:
            r = Xfer_Poll(now);
            if (r == 1U)
            {
                if (Parse(&raw_rh) != FALSE)
                {
                    /* --- Conversion (datasheet formulas, integer math) -----
                     * RH   [%]   = -6    + 125    * S / 2^16
                     * T    [degC]= -46.85 + 175.72 * S / 2^16
                     * Scaled x100 so we keep 2 decimals without floats.
                     * Max intermediate: 17572 * 65532 ~ 1.15e9 < 2^31 (OK).  */
                    rh = -600L + (int32_t)(((int32_t)12500 * raw_rh) / 65536L);
                    if (rh < 0L)     { rh = 0L;     }   /* datasheet: clamp   */
                    if (rh > 10000L) { rh = 10000L; }

                    t  = -4685L + (int32_t)(((int32_t)17572 * s_raw_t) / 65536L);

                    s_temp_c100 = (int16_t)t;
                    s_rh_c100   = (uint16_t)rh;
                    s_valid     = TRUE;        /* full good cycle -> valid    */
                    s_fail_cnt  = 0U;
                    s_state     = S_TRIG_T;    /* restart the cycle           */
                }
                else
                {
                    Fail(now);                 /* CRC error                   */
                }
            }
            else if (r == 2U)
            {
                Fail(now);
            }
            break;

        /* ======================== ERROR RECOVERY ========================== */
        case S_BACKOFF:
            if ((uint32_t)(now - s_t0) >= SHT21_BACKOFF_MS)
            {
                s_state = S_TRIG_T;            /* try again from scratch      */
            }
            break;

        default:
            s_state = S_TRIG_T;
            break;
    }
}

void Sht21_GetData(int16_t *temp_c100, uint16_t *rh_c100)
{
    *temp_c100 = s_temp_c100;
    *rh_c100   = s_rh_c100;
}

uint8_t Sht21_IsValid(void)
{
    return s_valid;
}
