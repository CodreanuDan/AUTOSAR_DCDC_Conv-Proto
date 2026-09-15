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

/**
 * Function name: SWC_SensorMeasurement_Runnable
 * @brief Periodic runnable that reads filtered physical measurements from RTE.
 * @param void
 * @return void
 */
void SWC_SensorMeasurement_Runnable(void);

/**
 * Function name: SWC_SensorMeasurement_GetVin
 * @brief Getter for Vin physical value.
 * @return float Input Voltage in Volts
 */
float SWC_SensorMeasurement_GetVin(void);

/**
 * Function name: SWC_SensorMeasurement_GetVout
 * @brief Getter for Vout physical value.
 * @return float Output Voltage in Volts
 */
float SWC_SensorMeasurement_GetVout(void);

/**
 * Function name: SWC_SensorMeasurement_GetIin
 * @brief Getter for Iin physical value.
 * @return float Input Current in Amperes
 */
float SWC_SensorMeasurement_GetIin(void);

/**
 * Function name: SWC_SensorMeasurement_GetIout
 * @brief Getter for Iout physical value.
 * @return float Output Current in Amperes
 */
float SWC_SensorMeasurement_GetIout(void);

/************* END OF FUNCTION PROTOTYPES ************/

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
}
#endif

#endif /* SWC_SENSOR_MEASUREMENT_H */

