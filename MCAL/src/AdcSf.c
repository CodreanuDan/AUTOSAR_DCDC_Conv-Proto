/*
 * Filename: AdcSf.c
 * @brief: Contains the function definitions for ADC Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "AdcSf.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/
/* Private MCAL internal buffers (Encapsulated, no global export) */
static volatile uint16_t s_adc_raw_results[ADC_NUM_CHANNELS] = {0U};
static volatile uint8_t  s_adc_current_channel = 0U;
static volatile bool     s_adc_scan_done = true;

/* Mapping physical ADC multiplexer pins (A0, A1, A2, A3) */
static const uint8_t s_adc_channel_map[ADC_NUM_CHANNELS] = { 0U, 1U, 2U, 3U };

/************* END OF VARIABLE DEFINITIONS *************/


/*******************************************************
 *               Local Function Prototypes
 */
void Adc_ISR_Routine(void);
/*************End of Local Function Prototypes *********/


/********************************************************
 *             START OF FUCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: Adc_Init
 * @brief Initializes the ADC hardware module.
 * Sets the voltage reference to AVcc with an external capacitor at the AREF pin.
 * Enables the ADC module, enables the ADC Conversion Complete Interrupt,
 * and sets the prescaler to 128 (ADPS[2:0] = 111) for maximum accuracy.
 * @param: void
 * @return: void
 */
void Adc_Init(void) 
{
    /* Select AVcc as voltage reference (REFS0 = 1, REFS1 = 0) */
    ADMUX  = (1U << REFS0);

    /* Enable ADC (ADEN), Enable ADC Interrupt (ADIE), set Prescaler to 128 (ADPS2..0 = 111) 
       16MHz / 128 = 125kHz ADC Clock (Within recommended 50kHz - 200kHz) */
    ADCSRA = (1U << ADEN) | (1U << ADIE) | (1U << ADPS2) | (1U << ADPS1) | (1U << ADPS0);

    s_adc_scan_done = true;
}

/**
 * Function name: Adc_StartScan
 * @brief Initiates a multi-channel ADC scan sequence starting from the first channel.
 * Checks if the previous scan cycle is complete. If so, clears the completion flag,
 * resets the channel index to 0, selects the corresponding pin for the first channel,
 * and starts the analog-to-digital conversion.
 * @param: void
 * @return: void
 */
void Adc_StartScan(void)
{
    /* Exit early if a scan sequence is already in progress */
    if (s_adc_scan_done == FALSE) 
    {
        return;
    }

    /* Reset scan status flag to mark scan as in progress */
    s_adc_scan_done = FALSE;

    /* Reset the channel index back to the first channel (channel 0) */
    s_adc_current_channel = 0U; 

    /* Clear existing channel bits (bits 0-3) in ADMUX and apply the new channel address */
    ADMUX = (ADMUX & 0xF0) | (s_adc_channel_map[0] & 0x0F);

    /* Start the first ADC conversion by setting the ADC Start Conversion bit (ADSC) */
    ADCSRA |= (1 << ADSC);
}


/*
 * Function name: Adc_IsScanDone
 * @brief Checks if all configured channels have finished sampling.
 * @param: void
 * @return: bool (TRUE if scan complete, FALSE if ongoing)
 */
bool Adc_IsScanDone(void)
{
    return s_adc_scan_done;
}


/*
 * Function name: Adc_ReadGroup
 * @brief Copies sampled raw ADC values into the provided destination buffer.
 * @param: uint16_t *buffer (Must have space for ADC_NUM_CHANNELS)
 * @return: void
 */
void Adc_ReadGroup(uint16_t *buffer)
{
    if (buffer != NULL)
    {
        /* Copy sampled values safely into IoHwAb buffer */
        for (uint8_t i = 0U; i < ADC_NUM_CHANNELS; i++)
        {
            buffer[i] = s_adc_raw_results[i];
        }
    }
}


/**
 * @brief Interrupt Service Routine (ISR) for ADC Conversion Complete.
 * 
 * Automatically executed when an analog-to-digital conversion finishes. Reads 
 * the raw ADC result, writes it to the local buffer and RTE interfaces, 
 * and either switches to the next channel to trigger a new conversion 
 * or completes the scan sequence.
 */
void Adc_ISR_Routine(void)
{
    /* Read 10-bit raw result from ADC register */
    s_adc_raw_results[s_adc_current_channel] = ADC;

    /* Move to the next channel in the sequence */
    s_adc_current_channel++;

    /* Check if there are more channels left to convert in the scan chain */
    if (g_adc_current_ch < ADC_NUM_CHANNELS) 
    {
        /* Select the hardware pin mapping for the next channel (preserve VREF configuration) */
        ADMUX = (ADMUX & 0xF0) | (adc_channel_map[g_adc_current_ch] & 0x0F);

        /* Start the conversion for the next channel */
        ADCSRA |= (1 << ADSC);
    } 
    else 
    {
        /* Mark the multi-channel scan cycle as finished */
        s_adc_scan_done = TRUE;
    }
}

/************* END OF FUCTION DEFINITIONS ************/


/******************* START OF ISR ********************/
ISR(ADC_vect) 
{
    Adc_ISR_Routine();
}

/********************** END OF ISR ********************/

/* End of AdcSf.c */