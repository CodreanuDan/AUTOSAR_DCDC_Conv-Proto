/*
 * Filename: ComSf.h
 * @brief: Contains the variables and function declaration for Communication module
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef COM_SF_H
#define COM_SF_H

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
#define COM_RX_FRAME_SIZE   12U


/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Com_MainFunction_Rx
 * @brief Periodically polls the UART ring buffer, parses incoming bytes,
 * validates frame checksums, and routes valid frames to DCM or PduR.
 * @param: void
 * @return: void
 */
void Com_MainFunction_Rx(void);

/**
 * Function name: Com_MainFunction_Tx
 * @brief Cyclic main function handling periodic and asynchronous message transmission.
 * Evaluates cyclic flags modified via DCM DIDs (0x0106 - 0x0110) before sending frames.
 * @param: void
 * @return: void
 */
void Com_MainFunction_Tx(void);

/* ----------------------FRAME TX FUNCTIONS -------------------------------------------*/

/**
 * Function name: Com_Send_DiagFrame_ConvMonitorData
 * @brief Packets raw ADC channel readings into a diagnostic frame (Header 0xAA)
 * and transmits it over UART along with an alive counter and calculated checksum.
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_ConvMonitorData(void);

/**
 * Function name: Com_Send_DiagFrame_PWMInfo
 * @brief Transmits hardware PWM status, OCR register values, and frequency (Header 0xA2).
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_PWMInfo(void);

/**
 * Function name: Com_Send_DiagFrame_PIDInfo
 * @brief Transmits PID controller parameters including target setpoint, output, and converter mode (Header 0xA3).
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_PIDInfo(void);

/**
 * Function name: Com_Send_DiagFrame_ActuatorInfo
 * @brief Transmits digital relay status for input and output actuators (Header 0xA4).
 * @param: void
 * @return: void
 */
void Com_Send_DiagFrame_ActuatorInfo(void);

/**
 * Function name: Com_Send_ActiveDTC_Frames
 * @brief Multi-frame transmission handler for active and passive Diagnostic Trouble Codes (Header 0xA5).
 * @param: void
 * @return: void
 */
void Com_Send_ActiveDTC_Frames(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* COM_SF_H */