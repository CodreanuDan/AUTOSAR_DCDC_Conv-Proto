/*
 * Filename: SWC_SensorMeasurement.h
 * @brief: Contains function prototypes for SWC_SensorMeasurement layer
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#ifndef SWC_SENSOR_MEASUREMENT_H
#define SWC_SENSOR_MEASUREMENT_H

/* Core libs */
#include <stdint.h>

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/*
 * Function name: SWC_SensorMeasurement_Runnable
 * @brief Periodically reads raw ADC data via RTE, applies moving average filters, and computes physical values.
 * @param: void
 * @return: void
 */
void SWC_SensorMeasurement_Runnable(void);

/*
 * Function name: SWC_SensorMeasurement_GetVin
 * @brief Returns the processed input voltage value.
 * @param: void
 * @return: float (Input voltage in volts)
 */
float SWC_SensorMeasurement_GetVin(void);

/*
 * Function name: SWC_SensorMeasurement_GetVout
 * @brief Returns the processed output voltage value.
 * @param: void
 * @return: float (Output voltage in volts)
 */
float SWC_SensorMeasurement_GetVout(void);

/************* END OF FUNCTION PROTOTYPES ************/

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
}
#endif

#endif /* SWC_SENSOR_MEASUREMENT_H */