/******************************************************************************
 * @file    I2cSf.h
 * @brief   MCAL I2C (TWI) master driver for ATmega328P - interface
 * @details Interrupt-driven, NON-BLOCKING master. The caller starts a
 *          transfer, returns immediately, and polls I2c_GetStatus() later
 *          (typically from a 10 ms task). All byte-level work is done in
 *          ISR(TWI_vect).
 *
 *          Layer rule: MCAL never uses RTE. Only BSW (e.g. Sht21Sf) calls this.
 *
 *          Pins: PC4 = SDA, PC5 = SCL (external 4.7k pull-ups to 5V).
 *          Speed: 100 kHz (fixed).
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 ******************************************************************************/
#ifndef I2C_SF_H
#define I2C_SF_H

/* Core libs */
#include <stdint.h>

/* C++ Guards for normal inclusion */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
 *            START OF VARIABLE DECLARATIONS
 *******************************************************/

/* I2C State machine possible States, Result of the last (or current) transfer */ 
typedef enum 
{ 
    I2C_IDLE = 0,     /* No transfer started yet / driver idle              */
    I2C_BUSY,         /* Transfer in progress, ISR is working on it         */
    I2C_OK,           /* Last transfer finished successfully                */
    I2C_ERR_NACK,     /* Slave NACKed address or data (absent / not ready)  */
    I2C_ERR_BUS       /* Unexpected TWI status (arbitration lost, bus error)
                         or transfer aborted by the caller                  */ 
} 
I2c_StatusType;

/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *             START OF FUNCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: I2c_Init
 * @brief   Sets 100 kHz clock and enables the TWI peripheral.
 *          Call once from EcuM_Init(), BEFORE sei().
 */
void           I2c_Init(void);

/**
 * Function name: I2c_StartWrite
 * @brief   Starts a master-write transfer: START, SLA+W, data[0..len-1], STOP.
 * @param   addr7 : 7-bit slave address (NOT shifted, e.g. 0x40)
 * @param   data  : buffer to send. MUST stay valid until the transfer ends
 *                  (use a static/global buffer, not a stack variable).
 * @param   len   : number of bytes (1..255)
 * @return  1 = accepted, 0 = rejected (driver busy or len == 0)
 */
uint8_t        I2c_StartWrite(uint8_t addr7, uint8_t *data, uint8_t len);  /* 1 = accepted */

/**
 * Function name: I2c_StartRead
 * @brief   Starts a master-read transfer: START, SLA+R, read len bytes
 *          (ACK all but the last, NACK the last), STOP.
 * @param   addr7 : 7-bit slave address
 * @param   data  : buffer that receives the bytes. Must stay valid until the
 *                  transfer ends.
 * @param   len   : number of bytes to read (1..255)
 * @return  1 = accepted, 0 = rejected (driver busy or len == 0)
 */
uint8_t        I2c_StartRead (uint8_t addr7, uint8_t *data, uint8_t len);  /* 1 = accepted */

/**
 * Function name: I2c_IsBusy
 * @brief   TRUE while a transfer is running OR a STOP condition is still
 *          being generated on the bus.
 */
uint8_t        I2c_IsBusy(void);

/**
 * Function name: I2c_GetStatus
 * @brief   Returns the status of the current/last transfer.
 */
I2c_StatusType I2c_GetStatus(void);

/**
 * Function name: I2c_Abort
 * @brief   Forces a STOP and releases the bus (used by upper layer on timeout,
 *          e.g. slave stretching the clock forever or SCL/SDA disconnected).
 *          Status becomes I2C_ERR_BUS.
 */
void           I2c_Abort(void);

/************* END OF FUNCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* I2C_SF_H */