/*
 * Filename: UartSf.c
 * @brief: Contains the function definitions for USART Driver
 * @author: Codreanu Dan ( Senior Project Arhitect: ComplicatedAsFuckEngineeringSolutions SRL (CAFES SRL))
 */

#include "UartSf.h"

/*******************************************************
 *            START OF VARIABLE DEFINITIONS
 *******************************************************/

/* Definitions for RX buffer */
volatile uint8_t s_rx_buf[UART_RX_BUF_SIZE];
volatile uint8_t s_rx_head = 0;
volatile uint8_t s_rx_tail = 0;

/* Definitions for TX buffer */
volatile uint8_t s_tx_buf[UART_TX_BUF_SIZE];
volatile uint8_t s_tx_head = 0;
volatile uint8_t s_tx_tail = 0;

/* Flag for signaling that TX buffer is busy */
volatile uint8_t s_tx_busy = 0;

/************* END OF VARIABLE DEFINITIONS *************/

/*******************************************************
 *               Local Function Prototypes
 */
static void Uart_RX_ISR_Routine(void);
static void Uart_TX_ISR_Routine(void);
/*************End of Local Function Prototypes *********/


/********************************************************
 *             START OF FUCTION DEFINITIONS
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
void Uart_Init(uint32_t baud)
{
    /* Calculate the Baud Rate Register value using standard speed mode (16x prescaler) */
    uint16_t ubrr = (uint16_t)(F_CPU / 16UL / baud - 1);
    /* Load upper 4 bits of the baud rate value into high register */
    UBRR0H = (uint8_t)(ubrr >> 8);
    /* Load lower 8 bits of the baud rate value into low register */
    UBRR0L = (uint8_t)ubrr;
    /* Enable Receiver (RXEN0), Transmitter (TXEN0), and RX Complete Interrupt (RXCIE0) */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    /* Set frame format: 8 data bits, no parity, 1 stop bit (8N1 configuration) */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8N1 */
}
 
/**
 * Function name: Uart_RxAvailable
 * @brief: Checks if there are any unread bytes available in the RX circular buffer.
 * Compares the head pointer/index with the tail pointer/index of the receive buffer.
 * @param: void
 * @return: uint8_t 1 (true) if unread data is available, 0 (false) if the buffer is empty.
 */
uint8_t Uart_RxAvailable(void)
{
    /* If head and tail indices are different, the buffer contains unread data */
    if (s_rx_head != s_rx_tail)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * Function name: Uart_RxByte
 * @brief: Reads and removes the oldest byte from the RX circular buffer.
 * Fetches the byte located at the current tail index, advances the tail index
 * using wrap-around modulo logic, and returns the retrieved byte.
 * @param: void
 * @return: byte (uint8_t) The next byte from the receive buffer.
 */
byte Uart_RxByte(void)
{
    /* Read the byte at the current tail position */
    uint8_t b = s_rx_buf[s_rx_tail];

    /* Advance the tail index and wrap around using modulo if reaching the buffer size */
    s_rx_tail = (s_rx_tail + 1) % UART_RX_BUF_SIZE;

    /* Return the retrieved byte */
    return b;
}

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
void Uart_TxByte(byte b)
{
    /* Calculate the next head index using wrap-around modulo arithmetic */
    uint8_t next = (s_tx_head + 1) % UART_TX_BUF_SIZE;

    /* Wait (blocking) as long as the transmit buffer is full */
    while (next == s_tx_tail) {}

    /* Write the byte to the current head position in the buffer */
    s_tx_buf[s_tx_head] = b;

    /* Update the head index to the next position */
    s_tx_head = next;

    /* If transmission interrupt loop is not active, start it */
    if (s_tx_busy == FALSE) 
    {
        s_tx_busy = TRUE;

        /* Enable the USART Data Register Empty Interrupt to trigger data sending */
        UCSR0B |= (1 << UDRIE0);
    }

}


/* Helpers for COM */

/**
 * Function name: Uart_GetRxBytesAvailable
 * @brief Returns the total number of unread bytes currently in the RX circular buffer.
 * @param: void
 * @return: uint16_t Total available bytes.
 */
uint16_t Uart_GetRxBytesAvailable(void)
{
    uint16_t bytes;

    if (s_rx_head >= s_rx_tail)
    {
        bytes = (uint16_t)(s_rx_head - s_rx_tail);
    }
    else
    {
        bytes = (uint16_t)(UART_RX_BUF_SIZE - s_rx_tail + s_rx_head);
    }

    return bytes;
}

/**
 * Function name: Uart_GetRxPeekIndex
 * @brief Calculates the buffer index at a given offset relative to the current tail.
 * @param: uint8_t offset (Offset from current tail, e.g., 0 to 11)
 * @return: uint8_t Calculated array index in s_rx_buf.
 */
uint8_t Uart_GetRxPeekIndex(uint8_t offset)
{
    return (uint8_t)((s_rx_tail + offset) % UART_RX_BUF_SIZE);
}

/**
 * Function name: Uart_GetRxBufferByte
 * @brief Reads a byte from the RX buffer at a specific index without advancing the tail.
 * @param: uint8_t index (Target array index in s_rx_buf)
 * @return: uint8_t Byte stored at the specified index.
 */
uint8_t Uart_GetRxBufferByte(uint8_t index)
{
    return s_rx_buf[index];
}



/* ISR Handlers */

/**
 * @brief Interrupt Service Routine (ISR) for USART Receive Complete (RX).
 * Automatically executed when a new byte is received hardware-side on UART0.
 * Reads the received byte from the data register (UDR0) and stores it into the 
 * RX circular buffer if the buffer is not full.
 */
static void Uart_RX_ISR_Routine(void)
{
    /* Read the incoming byte from the hardware register (clears RX flags) */
    uint8_t b = UDR0;

    /* Calculate the next head position with wrap-around modulo arithmetic */
    uint8_t next = (s_rx_head + 1) % UART_RX_BUF_SIZE;

    /* Check if the buffer is not full before storing the new byte */
    if (next != s_rx_tail) 
    {
        /* Store the received byte in the buffer at the current head index */
        s_rx_buf[s_rx_head] = b;

        /* Update the head index to point to the next free slot */
        s_rx_head = next;
    }
    /* Note: If the buffer is full (next == s_rx_tail), the byte 'b' is dropped */
}


/**
 * @brief Interrupt Service Routine (ISR) for USART Data Register Empty (UDRE).
 * Automatically executed when the UART hardware data register (UDR0) is ready 
 * to accept new data for transmission. Checks the TX circular buffer and either 
 * sends the next pending byte or disables the interrupt if the buffer is empty.
 */
static void Uart_TX_ISR_Routine(void)
{
    /* Check if the transmit circular buffer is empty */
    if (s_tx_head == s_tx_tail) 
    {
        /* Disable the Data Register Empty Interrupt to prevent continuous firing */
        UCSR0B &= ~(1 << UDRIE0);

        /* Mark transmission status as idle */
        s_tx_busy = FALSE;
    } 
    else 
    {
        /* Load the next byte from the buffer into the hardware transmission register */
        UDR0 = s_tx_buf[s_tx_tail];

        /* Advance the tail index and wrap around using modulo arithmetic */
        s_tx_tail = (s_tx_tail + 1) % UART_TX_BUF_SIZE;
    }
}

/************* END OF FUCTION DEFINITIONS ************/

/******************* START OF ISR ********************/
ISR(USART_RX_vect)
{
    Uart_RX_ISR_Routine();
}


ISR(USART_UDRE_vect)
{
    Uart_TX_ISR_Routine();
}

/********************** END OF ISR ********************/

/* End of UartSf.c */