/******************************************************************************
 * @file    I2cSf.c
 * @brief   MCAL I2C (TWI) master driver - implementation
 *
 *  How it works (big picture):
 *   1. I2c_StartWrite/Read() stores the transfer parameters in static
 *      variables and sets the START bit in TWCR. Then it returns at once.
 *   2. After every bus event the hardware sets TWINT and raises the TWI
 *      interrupt. ISR(TWI_vect) reads the status code (TWSR), decides the
 *      next action (send address / data, ACK / NACK, STOP) and writes TWCR
 *      (writing TWINT=1 "releases" the hardware for the next step).
 *   3. When the transfer ends, the ISR sets s_status = I2C_OK or an error and
 *      stops re-enabling the interrupt (TWIE = 0).
 *   4. The upper layer polls I2c_GetStatus().
 * @author: Codreanu Dan ( Senior Project Architect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 ******************************************************************************/

/* Core libs */
#include "I2cSf.h"
#include "StdTypes.h"

/* Project specific libs */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/* SCL = F_CPU / (16 + 2 * TWBR * prescaler). With prescaler = 1:
 * TWBR = (F_CPU / SCL - 16) / 2.  For 16 MHz and 100 kHz -> 72. */
#define I2C_TWBR_100K   ((uint8_t)((F_CPU / 100000UL - 16UL) / 2UL))   

/* ---- Driver state. 'volatile' because it is shared between ISR and main -- */
static volatile I2c_StatusType s_status = I2C_IDLE; /* result of transfer     */
static volatile uint8_t        s_addr_rw;           /* (addr7<<1) | R/W bit   */
static volatile uint8_t       *s_buf;               /* user data buffer       */
static volatile uint8_t        s_len;               /* total bytes to transfer*/
static volatile uint8_t        s_idx;               /* bytes done so far      */

/* ---- Helper macros for TWCR (the TWI control register) -------------------
 * TWINT : writing 1 clears the flag and lets the hardware proceed
 * TWEN  : TWI enable
 * TWIE  : TWI interrupt enable
 * TWEA  : send ACK after receiving a byte
 * TWSTA : generate START
 * TWSTO : generate STOP (cleared automatically by hardware when done)       */
#define TW_GO(ack)  (TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE) | ((ack) ? (1<<TWEA) : 0))
/* STOP: no TWIE, we are finished - the ISR must not fire again              */
#define TW_STOP()   (TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO))

/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *             START OF FUNCTION DEFINITIONS
 *******************************************************/

/**
 * Function name: I2c_Init
 */
void I2c_Init(void)
{
    TWSR = 0U;                   /* prescaler = 1 (TWPS1:0 = 0)              */
    TWBR = I2C_TWBR_100K;        /* bit rate -> 100 kHz                      */
    TWCR = (1<<TWEN);            /* enable TWI, interrupt still disabled     */
    s_status = I2C_IDLE;
}

/**
 * Function name: I2c_IsBusy
 * @note  TWSTO stays 1 until the STOP condition has really been sent on the
 *        bus. Starting a new transfer before that would corrupt the bus.
 */
uint8_t I2c_IsBusy(void)
{
    return (uint8_t)((s_status == I2C_BUSY) || (TWCR & (1<<TWSTO)));
}

/**
 * Function name: I2c_GetStatus
 */
I2c_StatusType I2c_GetStatus(void)
{
    return s_status;
}

/**
 * Function name: I2c_Start (internal)
 * @brief   Common part of StartWrite / StartRead.
 */
static uint8_t I2c_Start(uint8_t addr_rw, uint8_t *data, uint8_t len)
{
	    if ((len == 0U) || (I2c_IsBusy() != 0U))
    {
        return 0U;               /* reject: nothing to do or driver in use   */
    }

	s_addr_rw = addr_rw;
    s_buf     = data;
    s_len     = len;
    s_idx     = 0U;
    s_status  = I2C_BUSY;

    /* Send START; ISR takes over from the "TW_START" event.                 */
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN) | (1<<TWIE);
    return 1U;
}

uint8_t I2c_StartWrite(uint8_t addr7, uint8_t *data, uint8_t len)
{
    return I2c_Start((uint8_t)((addr7 << 1) | TW_WRITE), data, len);
}

uint8_t I2c_StartRead(uint8_t addr7, uint8_t *data, uint8_t len)
{
    return I2c_Start((uint8_t)((addr7 << 1) | TW_READ), data, len);
}

/**
 * Function name: I2c_Abort
 * @brief   Emergency exit: release the bus, report error.
 */
void I2c_Abort(void)
{
    TW_STOP();                   /* also disables the interrupt (TWIE = 0)   */
    s_status = I2C_ERR_BUS;
}

/******************************************************************************
 * TWI interrupt: the transfer state machine.
 * TW_STATUS = TWSR with prescaler bits masked -> one of the TW_* codes.
 ******************************************************************************/
ISR(TWI_vect)
{
    switch (TW_STATUS)
    {
		/* ---- START has been sent: transmit SLA+R/W ------------------------ */
		case TW_START:
		case TW_REP_START:
			TWDR = s_addr_rw;
			TW_GO(0);            
			break;
		/* ---------------------- MASTER TRANSMITTER ------------------------ */
		case TW_MT_SLA_ACK:      /* address acknowledged -> send 1st byte    */
		case TW_MT_DATA_ACK:     /* previous byte acknowledged -> next byte  */
			if (s_idx < s_len)
			{
				TWDR = s_buf[s_idx++];
				TW_GO(0);
			}
			else                 /* everything sent -> finish                */
			{
				TW_STOP();
				s_status = I2C_OK;
			}
			break;
		/* ----------------------- MASTER RECEIVER -------------------------- */
		case TW_MR_SLA_ACK:      /* address acknowledged, 1st byte is coming.
									If we want only ONE byte we must NACK it,
									otherwise ACK so the slave sends more.   */
			TW_GO(s_len > 1U);
			break;
		case TW_MR_DATA_ACK:     /* byte received and we ACKed it            */
			s_buf[s_idx++] = TWDR;
			/* ACK again only if MORE than one byte is still expected;
			 * the very last byte must be NACKed (tells slave to stop sending).*/
			TW_GO((uint8_t)(s_len - s_idx) > 1U);
			break;
		case TW_MR_DATA_NACK:    /* last byte received (we NACKed it)        */
			s_buf[s_idx++] = TWDR;
			TW_STOP();
			s_status = I2C_OK;
			break;
		/* ----------------------------- ERRORS ----------------------------- */
		case TW_MT_SLA_NACK:     /* no device at this address / not ready    */
		case TW_MR_SLA_NACK:     /* (SHT21 does this if conversion not done) */
		case TW_MT_DATA_NACK:    /* slave refused a data byte                */
			TW_STOP();
			s_status = I2C_ERR_NACK;
			break;
		default:                 /* arbitration lost, bus error, ...         */
			TW_STOP();
			s_status = I2C_ERR_BUS;
			break;
	}
}
/************* END OF FUNCTION PROTOTYPES ************/
