/* Header file with definitions and functions for Brainlink's bluetooth and auxiliary serial ports. More detail in uart.c */

#define BT_USART USARTD1
#define AUX_USART USARTC0

/*! \brief Enable USART receiver.
 *
 *  \param _usart    Pointer to the USART module
 */
#define USART_Rx_Enable(_usart) ((_usart)->CTRLB |= USART_RXEN_bm)


/*! \brief Disable USART receiver.
 *
 *  \param _usart Pointer to the USART module.
 */
#define USART_Rx_Disable(_usart) ((_usart)->CTRLB &= ~USART_RXEN_bm)


/*! \brief Enable USART transmitter.
 *
 *  \param _usart Pointer to the USART module.
 */
#define USART_Tx_Enable(_usart)	((_usart)->CTRLB |= USART_TXEN_bm)


/*! \brief Disable USART transmitter.
 *
 *  \param _usart Pointer to the USART module.
 */
#define USART_Tx_Disable(_usart) ((_usart)->CTRLB &= ~USART_TXEN_bm)

/*! \brief Macro that sets the USART frame format.
 *
 *  Sets the frame format, Frame Size, parity mode and number of stop bits.
 *
 *  \param _usart        Pointer to the USART module
 *  \param _charSize     The character size. Use USART_CHSIZE_t type.
 *  \param _parityMode   The parity Mode. Use USART_PMODE_t type.
 *  \param _twoStopBits  Enable two stop bit mode. Use bool type.
 */
#define USART_Format_Set(_usart, _charSize, _parityMode, _twoStopBits)         \
	(_usart)->CTRLC = (uint8_t) _charSize | _parityMode |                      \
	                  (_twoStopBits ? USART_SBMODE_bm : 0)


/* set mode */
#define USART_SetMode(_usart, _usartMode)                                      \
         ((_usart)->CTRLC = ((_usart)->CTRLC & (~USART_CMODE_gm)) | _usartMode)

/*! \brief Set USART baud rate.
 *
 *  Sets the USART's baud rate register.
 *
 *  UBRR_Value   : Value written to UBRR
 *  ScaleFactor  : Time Base Generator Scale Factor
 *
 *  Equation for calculation of BSEL value in asynchronous normal speed mode:
 *  	If ScaleFactor >= 0
 *  		BSEL = ((I/O clock frequency)/(2^(ScaleFactor)*16*Baudrate))-1
 *  	If ScaleFactor < 0
 *  		BSEL = (1/(2^(ScaleFactor)*16))*(((I/O clock frequency)/Baudrate)-1)
 *
 *	\note See XMEGA manual for equations for calculation of BSEL value in other
 *        modes.
 *
 *  \param _usart          Pointer to the USART module.
 *  \param _bselValue      Value to write to BSEL part of Baud control register.
 *                         Use uint16_t type.
 *  \param _bScaleFactor   USART baud rate scale factor.
 *                         Use uint8_t type
 */
#define USART_Baudrate_Set(_usart, _bselValue, _bScaleFactor)                  \
	(_usart)->BAUDCTRLA =(uint8_t)_bselValue;                                           \
	(_usart)->BAUDCTRLB =(_bScaleFactor << USART_BSCALE0_bp)|(_bselValue >> 8)
	

/*! \brief Checks if the RX complete interrupt flag is set.
 *
 *   Checks if the RX complete interrupt flag is set.
 *
 *  \param _usart     The USART module.
 */
#define USART_IsRXComplete(_usart) (((_usart)->STATUS & USART_RXCIF_bm) != 0)


/*! \brief Check if data register empty flag is set.
 *
 *  \param _usart      The USART module.
 */
#define USART_IsTXDataRegisterEmpty(_usart) (((_usart)->STATUS & USART_DREIF_bm) != 0)



/*! \brief Set USART RXD interrupt level.
 *
 *  Sets the interrupt level on RX Complete interrupt.
 *
 *  \param _usart        Pointer to the USART module.
 *  \param _rxdIntLevel  Interrupt level of the RXD interrupt.
 *                       Use USART_RXCINTLVL_t type.
 */
#define USART_RxdInterruptLevel_Set(_usart, _rxdIntLevel)                      \
	((_usart)->CTRLA = ((_usart)->CTRLA & ~USART_RXCINTLVL_gm) | _rxdIntLevel)

// Define a buffer size of 256 for received data
#define USART_RX_BUFFER_SIZE 256
/* \brief Receive buffer mask. */
#define USART_RX_BUFFER_MASK ( USART_RX_BUFFER_SIZE - 1 )

/* \brief USART receive ring buffer. */
typedef struct USART_Buffer
{
	/* \brief Receive buffer. */
	volatile uint8_t RX[USART_RX_BUFFER_SIZE];
	/* \brief Transmit buffer - UNUSED */
	//volatile uint8_t TX[USART_TX_BUFFER_SIZE];
	/* \brief Receive buffer head. */
	volatile uint8_t RX_Head;
	/* \brief Receive buffer tail. */
	volatile uint8_t RX_Tail;
	/* \brief Transmit buffer head. */
	//volatile uint8_t TX_Head;
	/* \brief Transmit buffer tail. */
	//volatile uint8_t TX_Tail;
} USART_Buffer_t;

/*! \brief Struct used when interrupt driven driver is used.
*
*  Struct containing pointer to a usart, a buffer and a location to store Data
*  register interrupt level temporary.
*/
typedef struct Usart_and_buffer
{
	/* \brief Pointer to USART module to use. */
	USART_t * usart;
	/* \brief Data register empty interrupt level. */
	//USART_DREINTLVL_t dreIntLevel;
	/* \brief Data buffer. */
	USART_Buffer_t buffer;
}USART_data_t;

FILE bt;
FILE aux;

/*! USART data struct used for bluetooth receive. */
USART_data_t BT_data;

/*! USART data struct used for aux receive. */
USART_data_t AUX_data;


void init_bt_uart(void);
void init_aux_uart(int scale, char baud);
void set_aux_baud_rate(int baud, char scale);
void uart_putchar(USART_t * usart, char c);
char uart_getchar(USART_t * usart);
int uart_getchar_timeout(USART_t * usart);
char aux_uart_getchar(USART_t * usart);
char uart_getchar_nb(USART_t * usart);
void USART_InterruptDriver_Initialize(USART_data_t * usart_data, USART_t * usart);
bool USART_RXBufferData_Available(USART_data_t * usart_data);
uint8_t USART_RXBuffer_GetByte(USART_data_t * usart_data);
bool USART_RXComplete(USART_data_t * usart_data);
void err(void);
void USART_RXBuffer_Clear(USART_data_t * usart_data);
void serial_bridge(void);
void init_aux_uart_ir(int baud, char scale);
void disable_aux_uart(void);
