/*
 * Filename: AdcSf.h
 * @brief: Contains the variables and function declaration for ADC Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef ADC_SF_H
#define ADC_SF_H

/* Core libs */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

/* Project specific libs */
#include "StdTypes.h"

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *            START OF VARIABLE DECLARATIONS
 *******************************************************/

#define ADC_NUM_CH  4U


/* Array of ADC Channels raw values */
extern uint16_t g_adc_raw[ADC_NUM_CH];

/* Current channel during reading */
extern volatile uint8_t g_adc_current_ch;

/* Flag to signal that an ADC Ch scan is completed */
extern volatile uint8_t g_adc_scan_done;

/* Array of ADC Channels used */
extern const uint8_t adc_channel_map[ADC_NUM_CH];

/************* END OF VARIABLE DECLARATIONS ************/

/*******************************************************
 *             START OF FUCTION PROTOTYPES
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
void Adc_Init(void);

/**
 * Function name: Adc_StartScan
 * @brief Initiates a multi-channel ADC scan sequence starting from the first channel.
 * Checks if the previous scan cycle is complete. If so, clears the completion flag,
 * resets the channel index to 0, selects the corresponding pin for the first channel,
 * and starts the analog-to-digital conversion.
 * @param: void
 * @return: void
 */
void Adc_StartScan(void);


/************* END OF FUCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* ADC_SF_H */