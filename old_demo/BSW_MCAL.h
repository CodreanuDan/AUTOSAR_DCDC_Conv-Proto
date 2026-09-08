/* BSW_MCAL.h */

#ifndef BSW_MCAL_H
#define BSW_MCAL_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef TRUE
#define TRUE  1U
#endif

#ifndef FALSE
#define FALSE 0U
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Prototipul Hook-ului de Timer pentru OS Scheduler */
void Os_TimerTick_Hook(void);

/* =====================================================================
 * 1. MICROCONTROLLER ABSTRACTION LAYER (MCAL) - Direct Register Access
 * ===================================================================== */

/* UART Driver */
#define UART_RX_BUF_SIZE 128
#define UART_TX_BUF_SIZE 128
#define FRAME_SIZE 12

extern volatile uint8_t s_rx_buf[UART_RX_BUF_SIZE];
extern volatile uint8_t s_rx_head;
extern volatile uint8_t s_rx_tail;
extern volatile uint8_t s_tx_buf[UART_TX_BUF_SIZE];
extern volatile uint8_t s_tx_head;
extern volatile uint8_t s_tx_tail;
extern volatile uint8_t s_tx_busy;

void Uart_Init(uint32_t baud);
uint8_t Uart_RxAvailable(void);
uint8_t Uart_RxByte(void);
void Uart_TxByte(uint8_t b);

/* ADC Driver */
#define ADC_NUM_CH   4
extern volatile uint16_t g_adc_raw[ADC_NUM_CH];
extern volatile uint8_t  g_adc_scan_done;

void Adc_Init(void);
void Adc_StartScan(void);

/* PWM & Control Globals */
extern volatile uint8_t duty_a;
extern volatile uint8_t duty_b;
extern volatile uint16_t target_frequency;
extern volatile uint16_t OCR1A_prev_val;
extern volatile uint16_t OCR1B_prev_val;
extern volatile uint16_t ICR1_prev_val;

/* GPT / System Tick Driver (Timer 0) */
extern volatile uint32_t g_tick_ms;
void Timer0_TickInit(void);

/* =====================================================================
 * 2. ECU ABSTRACTION & BSW SERVICES LAYER - Data Processing
 * ===================================================================== */

/* Sensor Conversion Functions (IoHwAb) */
float Acs712_Drv_ConvertToAmps(uint16_t raw);
float Vdiv_Drv_ReadVoltage(uint16_t raw);

/* Moving Average Filters */
#define FILTER_WINDOW_SIZE 16
typedef struct {
    float buffer[FILTER_WINDOW_SIZE];
    uint8_t idx;
    uint8_t filled;
    float sum;
} MovAvg_HandleType;

extern MovAvg_HandleType s_filt_iin;
extern MovAvg_HandleType s_filt_iout;

void MovAvg_Init(MovAvg_HandleType *f);
float MovAvg_Update(MovAvg_HandleType *f, float new_sample);
void Filters_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* BSW_MCAL_H */