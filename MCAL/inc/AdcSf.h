/*
 * Filename: AdcSf.h
 * @brief: Contains the variables and function declaration for ADC Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef ADC_SF_H
#define ADC_SF_H

/* Core libs */
#include <stdint.h>
#include <stdbool.h>

/* Project specific libs */
#include "StdTypes.h"

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *            TYPE DEFINITIONS & MACROS
 *******************************************************/
#define ADC_NUM_CHANNELS  4U

#define ADC_CH_IIN   0U
#define ADC_CH_VIN   1U
#define ADC_CH_IOUT  2U
#define ADC_CH_VOUT  3U

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


/*
 * Function name: Adc_IsScanDone
 * @brief Checks if all configured channels have finished sampling.
 * @param: void
 * @return: bool (TRUE if scan complete, FALSE if ongoing)
 */
bool Adc_IsScanDone(void);

/*
 * Function name: Adc_ReadGroup
 * @brief Copies sampled raw ADC values into the provided destination buffer.
 * @param: uint16_t *buffer (Must have space for ADC_NUM_CHANNELS)
 * @return: void
 */
void Adc_ReadGroup(uint16_t *buffer);


/************* END OF FUCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* ADC_SF_H */