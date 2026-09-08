/* BSW_MCAL.c */

#include "BSW_MCAL.h"
#include "Rte.h"   // Legatura cu stratul superior RTE

// #ifdef __cplusplus
// extern "C" {
// #endif

/* Adauga la începutul BSW_MCAL.c (dupa #include-uri) */
volatile uint16_t g_adc_raw[ADC_NUM_CH] = {0};
volatile uint8_t duty_a = 98;
volatile uint8_t duty_b = 98;
volatile uint16_t target_frequency = 250;
volatile uint16_t OCR1A_prev_val = 0;
volatile uint16_t OCR1B_prev_val = 0;
volatile uint16_t ICR1_prev_val = 0;

/* =====================================================================
 * 1. MCAL IMPLEMENTATIONS
 * ===================================================================== */

/* UART Driver */
volatile uint8_t s_rx_buf[UART_RX_BUF_SIZE];
volatile uint8_t s_rx_head = 0;
volatile uint8_t s_rx_tail = 0;

volatile uint8_t s_tx_buf[UART_TX_BUF_SIZE];
volatile uint8_t s_tx_head = 0;
volatile uint8_t s_tx_tail = 0;
volatile uint8_t s_tx_busy = 0;

void Uart_Init(uint32_t baud)
{
    uint16_t ubrr = (uint16_t)(F_CPU / 16UL / baud - 1);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8N1 */
}

uint8_t Uart_RxAvailable(void) { return (s_rx_head != s_rx_tail); }

uint8_t Uart_RxByte(void)
{
    uint8_t b = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % UART_RX_BUF_SIZE;
    return b;
}

void Uart_TxByte(uint8_t b)
{
    uint8_t next = (s_tx_head + 1) % UART_TX_BUF_SIZE;
    while (next == s_tx_tail) {}
    s_tx_buf[s_tx_head] = b;
    s_tx_head = next;
    if (!s_tx_busy) 
    {
        s_tx_busy = 1;
        UCSR0B |= (1 << UDRIE0);
    }
}

ISR(USART_RX_vect)
{
    uint8_t b = UDR0;
    uint8_t next = (s_rx_head + 1) % UART_RX_BUF_SIZE;
    if (next != s_rx_tail) 
    {
        s_rx_buf[s_rx_head] = b;
        s_rx_head = next;
    }
}

ISR(USART_UDRE_vect)
{
    if (s_tx_head == s_tx_tail) 
    {
        UCSR0B &= ~(1 << UDRIE0);
        s_tx_busy = 0;
    } 
    else 
    {
        UDR0 = s_tx_buf[s_tx_tail];
        s_tx_tail = (s_tx_tail + 1) % UART_TX_BUF_SIZE;
    }
}

/* ADC Driver & RTE Data Propagation */
extern volatile uint16_t g_adc_raw[ADC_NUM_CH];
volatile uint8_t  g_adc_current_ch = 0;
volatile uint8_t  g_adc_scan_done = 1;
const uint8_t adc_channel_map[ADC_NUM_CH] = { 0, 1, 2, 3 };

void Adc_Init(void) 
{
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void Adc_StartScan(void)
{
    if (!g_adc_scan_done) return;
    g_adc_scan_done = 0;
    g_adc_current_ch = 0; 
    ADMUX = (ADMUX & 0xF0) | (adc_channel_map[0] & 0x0F);
    ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect) 
{
    uint16_t val = ADC;
    g_adc_raw[g_adc_current_ch] = val;

    /* Propagam datele direct prin RTE catre straturile superioare */
    if      (g_adc_current_ch == 0) Rte_Write_AdcRaw_Iin(val);
    else if (g_adc_current_ch == 1) Rte_Write_AdcRaw_Vin(val);
    else if (g_adc_current_ch == 2) Rte_Write_AdcRaw_Iout(val);
    else if (g_adc_current_ch == 3) Rte_Write_AdcRaw_Vout(val);

    g_adc_current_ch++;

    if (g_adc_current_ch < ADC_NUM_CH) 
    {
        ADMUX = (ADMUX & 0xF0) | (adc_channel_map[g_adc_current_ch] & 0x0F);
        ADCSRA |= (1 << ADSC);
    } 
    else 
    {
        g_adc_scan_done = 1;
        Rte_Write_AdcScanDone(1);
    }
}

/* Timer0 GPT Driver */
volatile uint32_t g_tick_ms = 0;
void Timer0_TickInit(void) 
{
    TCCR0A = (1 << WGM01);
    OCR0A  = 249;
    TCCR0B = (1 << CS01) | (1 << CS00);
    TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) 
{
    g_tick_ms++;
    Rte_Write_TickMs(g_tick_ms);
    Os_TimerTick_Hook();  /* <--- Adauga aceasta linie! */
}


/* =====================================================================
 * 2. ECU ABSTRACTION & BSW IMPLEMENTATIONS
 * ===================================================================== */

float Acs712_Drv_ConvertToAmps(uint16_t raw) 
{
    float v = ((float)raw / 1023.0f) * 5.0f;
    return (v - 2.5f) / 0.066f;
}

float Vdiv_Drv_ReadVoltage(uint16_t raw) 
{
    float v_adc = ((float)raw / 1023.0f) * 5.0f;
    return v_adc * 12.0f;
}

MovAvg_HandleType s_filt_iin;
MovAvg_HandleType s_filt_iout;

void Filters_Init(void) 
{     
    MovAvg_Init(&s_filt_iin);
    MovAvg_Init(&s_filt_iout); 
}

void MovAvg_Init(MovAvg_HandleType *f) 
{
	uint8_t i;
    for (i = 0; i < FILTER_WINDOW_SIZE; i++) f->buffer[i] = 0.0f;
    f->idx = 0;
    f->filled = 0;
    f->sum = 0.0f;
}


float MovAvg_Update(MovAvg_HandleType *f, float new_sample) 
{
    f->sum -= f->buffer[f->idx];
    f->buffer[f->idx] = new_sample;
    f->sum += new_sample;
    f->idx = (f->idx + 1) % FILTER_WINDOW_SIZE;
    if (f->filled < FILTER_WINDOW_SIZE) f->filled++;
    return f->sum / f->filled;
}

// #ifdef __cplusplus
// }
// #endif