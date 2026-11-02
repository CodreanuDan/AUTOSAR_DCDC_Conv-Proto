/*
 * Filename: UartSf.h
 * @brief: Contains the variables and function declaration for USART Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL)) 
 */

#ifndef UART_SF_H
#define UART_SF_H

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

/* Common types for USART communication driver */
#define DEFAULT_BAUD_RATE 57600U
typedef uint8_t byte;


/* Size set for TX and RX buffers */
#define UART_RX_BUF_SIZE 128U
#define UART_TX_BUF_SIZE 128U

/* Size set for standard UDS frame*/
#define FRAME_SIZE 12U

/* Declarations for RX buffer */
extern volatile uint8_t s_rx_buf[UART_RX_BUF_SIZE];
extern volatile uint8_t s_rx_head;
extern volatile uint8_t s_rx_tail;

/* Declarations for TX buffer */
extern volatile uint8_t s_tx_buf[UART_TX_BUF_SIZE];
extern volatile uint8_t s_tx_head;
extern volatile uint8_t s_tx_tail;

/* Flag for signaling that TX buffer is busy */
extern volatile uint8_t s_tx_busy;

/************* END OF VARIABLE DECLARATIONS ************/

/*******************************************************
 *             START OF FUCTION PROTOTYPES
 *******************************************************/

/**
 * Function name: Uart_Init
 * @brief: Initializes the UART0 hardware module.
 * Configures the baud rate register based on the system clock frequency (F_CPU),
 * enables receiver and transmitter functionality, enables the RX complete interrupt,
 * and sets the frame format to 8 data bits, no parity, and 1 stop bit (8N1).
 * @param: uint32_t baud (Target communication speed in bits per second (default 57600).)
 * @return: void
 */
void Uart_Init(uint32_t baud);

/**
 * Function name: Uart_RxAvailable
 * @brief: Checks if there are any unread bytes available in the RX circular buffer.
 * Compares the head pointer/index with the tail pointer/index of the receive buffer.
 * @param: void
 * @return: uint8_t 1 (true) if unread data is available, 0 (false) if the buffer is empty.
 */
uint8_t Uart_RxAvailable(void);

/**
 * Function name: Uart_RxByte
 * @brief: Reads and removes the oldest byte from the RX circular buffer.
 * Fetches the byte located at the current tail index, advances the tail index
 * using wrap-around modulo logic, and returns the retrieved byte.
 * @param: void
 * @return: byte (uint8_t) The next byte from the receive buffer.
 */
byte Uart_RxByte(void);

/**
 * Function name: Uart_TxByte
 * @brief: Enqueues a byte into the TX circular buffer and initiates transmission.
 * Checks if space is available in the buffer (blocks if full), writes the byte 
 * at the current head position, and advances the head index. If transmission is 
 * not currently active, marks the TX module as busy and enables the USART Data 
 * Register Empty Interrupt (UDRIE0) to start sending data.
 * @param: byte b The byte to be transmitted via UART.
 * @return: void
 */
void Uart_TxByte(byte b);


/**
 * Function name: Uart_GetRxBytesAvailable
 * @brief Returns the total number of unread bytes currently in the RX circular buffer.
 * @param: void
 * @return: uint16_t Total available bytes.
 */
uint16_t Uart_GetRxBytesAvailable(void);


/**
 * Function name: Uart_GetRxPeekIndex
 * @brief Calculates the buffer index at a given offset relative to the current tail.
 * @param: uint8_t offset (Offset from current tail, e.g., 0 to 11)
 * @return: uint8_t Calculated array index in s_rx_buf.
 */
uint8_t Uart_GetRxPeekIndex(uint8_t offset);

/**
 * Function name: Uart_GetRxBufferByte
 * @brief Reads a byte from the RX buffer at a specific index without advancing the tail.
 * @param: uint8_t index (Target array index in s_rx_buf)
 * @return: uint8_t Byte stored at the specified index.
 */
uint8_t Uart_GetRxBufferByte(uint8_t index);


/************* END OF FUCTION PROTOTYPES ************/

#ifdef __cplusplus
}
#endif

#endif /* UART_SF_H */