/*
 * Filename: IoHwAb.h
 * @brief: Contains hardware abstraction definitions, filter configurations, and sensor conversion prototypes
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#ifndef IOHWAB_H
#define IOHWAB_H

#include <stdint.h>
#include "StdTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *            CONFIGURATIONS & MACRO DEFINITIONS
 *******************************************************/
/* Filter window size set to 16 samples for optimal balance between noise suppression and step response latency */
#define FILTER_WINDOW_SIZE      16U

/* ADC 10-bit maximum digital value mapping to reference voltage */
#define ADC_MAX_RESOLUTION      1023.0f

/* Microcontroller ADC reference voltage (AVCC = 5.0V) */
#define ADC_VREF_VOLTS          5.0f

/* ACS712-30A Current Sensor: 2.5V represents 0A (bidirectional offset) */
#define ACS712_ZERO_OFFSET_V    2.5f

/* ACS712-30A Current Sensor Sensitivity: 66 mV per Ampere */
#define ACS712_SENSITIVITY      0.066f

/* Voltage Divider Scaling Ratio: Scaled down to match 0-5V ADC range */
#define VOLTAGE_DIVIDER_RATIO   12.0f

/*******************************************************
 *            TYPE DEFINITIONS
 *******************************************************/
typedef struct {
    float buffer[FILTER_WINDOW_SIZE];
    uint8_t idx;
    uint8_t filled;
    float sum;
} MovAvg_HandleType;

/*******************************************************
 *             FUNCTION PROTOTYPES
 *******************************************************/

 /*
 * Function name: MovAvg_Init
 * @brief Resets history buffer and operational variables for a specific moving average filter instance.
 * @param: MovAvg_HandleType *f (Pointer to filter instance)
 * @return: void
 */
void Filters_Init(void);

/*
 * Function name: Filters_Init
 * @brief Initializes moving average filter instances for all monitored signals.
 * @param: void
 * @return: void
 */
void MovAvg_Init(MovAvg_HandleType *f);

/*
 * Function name: MovAvg_Update
 * @brief Pushes a new raw sample into the sliding window and computes the smoothed arithmetic mean.
 * @param: MovAvg_HandleType *f, float new_sample
 * @return: float smoothed output value
 */
float MovAvg_Update(MovAvg_HandleType *f, float new_sample);

/*
 * Function name: IoHwAb_Analog_ConvertToAmps
 * @brief Converts raw 10-bit counts to Amperes using ACS712 sensor offsets and sensitivity calibration constants.
 * @param: uint16_t raw
 * @return: float calculated current in Amperes
 */
float IoHwAb_Analog_ConvertToAmps(uint16_t raw);

/*
 * Function name: IoHwAb_Analog_ReadVoltage
 * @brief Converts raw 10-bit counts to Volts using the hardware resistor divider scaling factor.
 * @param: uint16_t raw
 * @return: float calculated voltage in Volts
 */
float IoHwAb_Analog_ReadVoltage(uint16_t raw);

/*
 * Function name: IoHwAb_Sensor_MainFunction
 * @brief Periodically processes raw ADC scans, executes noise filtering on currents and voltages, and updates RTE.
 * @param: void
 * @return: void
 */
void IoHwAb_Sensor_MainFunction(void);

/*
 * Function name: IoHwAb_Actuator_MainFunction
 * @brief Translates RTE logical actuator requests into direct MCAL hardware executions.
 * @param: void
 * @return: void
 */
void IoHwAb_Actuator_MainFunction(void);


/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* IOHWAB_H */
