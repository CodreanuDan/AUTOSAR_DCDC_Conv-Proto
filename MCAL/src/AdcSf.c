/*
 * Filename: AdcSf.c
 * @brief: Contains the function definitions for ADC Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "AdcSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/
/* Array of ADC Channels raw values */
uint16_t g_adc_raw[ADC_NUM_CH];

/* Current channel during reading */
volatile uint8_t g_adc_current_ch = 0;

/* Flag to signal that an ADC Ch scan is completed */
volatile uint8_t g_adc_scan_done = TRUE;

/* Array of ADC Channels mapping */
const uint8_t adc_channel_map[ADC_NUM_CH] = { 0, 1, 2, 3 };

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
    ADMUX  = (1 << REFS0);

    /* Enable ADC (ADEN), Enable ADC Interrupt (ADIE), set Prescaler to 128 (ADPS2:0) */
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
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
    if (g_adc_scan_done == FALSE) 
    {
        return;
    }

    /* Reset scan status flag to mark scan as in progress */
    g_adc_scan_done = FALSE;

    /* Reset the channel index back to the first channel (channel 0) */
    g_adc_current_ch = 0; 

    /* Clear existing channel bits (bits 0-3) in ADMUX and apply the new channel address */
    ADMUX = (ADMUX & 0xF0) | (adc_channel_map[0] & 0x0F);

    /* Start the first ADC conversion by setting the ADC Start Conversion bit (ADSC) */
    ADCSRA |= (1 << ADSC);
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
    /* Read the 10-bit raw ADC result from the hardware register */
    uint16_t val = ADC;

    /* Store the raw reading in the global buffer for the current channel */
    g_adc_raw[g_adc_current_ch] = val;

    /* Propagate the conversion results directly to upper software layers via RTE */
    if      (g_adc_current_ch == 0) Rte_Write_AdcRaw_Iin(val);
    else if (g_adc_current_ch == 1) Rte_Write_AdcRaw_Vin(val);
    else if (g_adc_current_ch == 2) Rte_Write_AdcRaw_Iout(val);
    else if (g_adc_current_ch == 3) Rte_Write_AdcRaw_Vout(val);

    /* Move to the next channel in the sequence */
    g_adc_current_ch++;

    /* Check if there are more channels left to convert in the scan chain */
    if (g_adc_current_ch < ADC_NUM_CH) 
    {
        /* Select the hardware pin mapping for the next channel (preserve VREF configuration) */
        ADMUX = (ADMUX & 0xF0) | (adc_channel_map[g_adc_current_ch] & 0x0F);

        /* Start the conversion for the next channel */
        ADCSRA |= (1 << ADSC);
    } 
    else 
    {
        /* Mark the multi-channel scan cycle as finished */
        g_adc_scan_done = TRUE;

        /* Notify upper software layers via RTE that scan data is ready */
        Rte_Write_AdcScanDone(1);
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