/* Source file with functions for Brainlink's bluetooth and auxiliary serial ports. */

// Initializes the serial port attached to Brainlink's bluetooth module
void init_bt_uart() {
	// set rx and tx pin directionality
	PORTD.DIRSET = PIN7_bm;
	PORTD.DIRCLR = PIN6_bm;

	/* USARTC0, 8 Data bits, No Parity, 1 Stop bit. */
	USART_Format_Set(&BT_USART, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc, false);

	/* Use USARTC0 and initialize buffers. */
	USART_InterruptDriver_Initialize(&BT_data, &BT_USART);

	/* Enable RXC interrupt. */
	USART_RxdInterruptLevel_Set(BT_data.usart, USART_RXCINTLVL_MED_gc);
	/* Enable PMIC interrupt level medium. */
	PMIC.CTRL |= PMIC_MEDLVLEX_bm;

	// Set baud rate to 115,200 using -3 for scale, 131 for setting.  Yields 0.08% error
	USART_Baudrate_Set(&BT_USART, 131 , -3);

	// Enable pins
	USART_Rx_Enable(&BT_USART);
	USART_Tx_Enable(&BT_USART);
}

// Initializes auxiliary serial port - variables required to set baud rate are passed to the function
void init_aux_uart(int baud, char scale) {
	// set rx and tx directionality
	PORTC.DIRSET = PIN3_bm;
	PORTC.DIRCLR = PIN2_bm;

	/* USARTC0, 8 Data bits, No Parity, 1 Stop bit. */
	USART_Format_Set(&AUX_USART, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc, false);

	/* Use USARTC0 and initialize buffers. */
	USART_InterruptDriver_Initialize(&AUX_data, &AUX_USART);

	/* Enable RXC interrupt. */
	USART_RxdInterruptLevel_Set(AUX_data.usart, USART_RXCINTLVL_MED_gc);
	/* Enable PMIC interrupt level medium. */
	PMIC.CTRL |= PMIC_MEDLVLEX_bm;
	// Set baud rate to that selected by user (defaults to 115200)
	USART_Baudrate_Set(&AUX_USART, baud, scale);

	// Enable pins
	USART_Rx_Enable(&AUX_USART);
	USART_Tx_Enable(&AUX_USART);
	
}

// Sets the baud rate if the aux serial port is already set up
void set_aux_baud_rate(int baud, char scale) 
{
	USART_Baudrate_Set(&AUX_USART, baud, scale);
}

// Used to send a byte over a serial port
void uart_putchar(USART_t * usart, char c) {

	do{
		/* Wait until it is possible to put data into TX data register.
		 * NOTE: If TXDataRegister never becomes empty this will be a DEADLOCK. */
	}while((usart->STATUS & USART_DREIF_bm) == 0);
	//while(!USART_IsTXDataRegisterEmpty(&usart));
	usart->DATA = c;
}

// Gets a character without blocking
char uart_getchar_nb(USART_t * usart) {

	int timeout = 1000;
	do{
	/* Wait until data received or a timeout.*/
	timeout--;
	}while(((usart->STATUS & USART_RXCIF_bm) == 0) && timeout!=0);
	
	if(timeout==0) {
		return 0;
	}
	else {
		return usart->DATA;
	}
}

// Gets a character from the bluetooth buffer, blocks if none exists. Not currently used.
char uart_getchar(USART_t * usart) {
	
//	do{
//	}while((usart->STATUS & USART_RXCIF_bm) == 0);
//	uart_putchar(usart, (usart->STATUS&0x18));//
	do{
    }while(!USART_RXBufferData_Available(&BT_data));
	//uart_putchar(usart, (usart->STATUS&0x18));//
	return USART_RXBuffer_GetByte(&BT_data);
}


// Gets a character from the bluetooth buffer, blocks if none exists for roughly 300 ms, then times out and returns 256
// Bug alert  - "USART_t * usart" is unnecessary and not used
int uart_getchar_timeout(USART_t * usart) {
	
	int timeout=0;
	while(!USART_RXBufferData_Available(&BT_data) && timeout < 500) {
		timeout++;
		_delay_ms(1);
	}
	if(timeout >= 500)
		return 256;
	else
		return USART_RXBuffer_GetByte(&BT_data);
}

// Gets a character from the auxiliary uart buffer, not currently used.
char aux_uart_getchar(USART_t * usart) {
	
	return USART_RXBuffer_GetByte(&AUX_data);
}

// serial to Bluetooth bridge
// currently does not return
void serial_bridge(void) {
    while(1) {
        while (USART_RXBufferData_Available(&AUX_data)) {
              uart_putchar(&BT_USART, USART_RXBuffer_GetByte(&AUX_data));
        }
        while (USART_RXBufferData_Available(&BT_data)) {
              uart_putchar(&AUX_USART, USART_RXBuffer_GetByte(&BT_data));
        }
    }
}

/*! \brief Initializes buffer and selects what USART module to use.
 *
 *  Initializes receive and transmit buffer and selects what USART module to use,
 *  and stores the data register empty interrupt level.
 *
 *  \param usart_data           The USART_data_t struct instance.
 *  \param usart                The USART module.
 *  \param dreIntLevel          Data register empty interrupt level.
 */
void USART_InterruptDriver_Initialize(USART_data_t * usart_data,
                                      USART_t * usart)
//                                      USART_DREINTLVL_t dreIntLevel)
{
	usart_data->usart = usart;
	//usart_data->dreIntLevel = dreIntLevel;

	usart_data->buffer.RX_Tail = 0;
	usart_data->buffer.RX_Head = 0;
	//usart_data->buffer.TX_Tail = 0;
	//usart_data->buffer.TX_Head = 0;
}

/*! \brief Test if there is data in the receive software buffer.
 *
 *  This function can be used to test if there is data in the receive software
 *  buffer.
 *
 *  \param usart_data         The USART_data_t struct instance
 *
 *  \retval true      There is data in the receive buffer.
 *  \retval false     The receive buffer is empty.
 */
bool USART_RXBufferData_Available(USART_data_t * usart_data)
{
	/* Make copies to make sure that volatile access is specified. */
	uint8_t tempHead = usart_data->buffer.RX_Head;
	uint8_t tempTail = usart_data->buffer.RX_Tail;

	/* There are data left in the buffer unless Head and Tail are equal. */
	return (tempHead != tempTail);
}



/*! \brief Get received data (5-8 bit character).
 *
 *  The function USART_RXBufferData_Available should be used before this
 *  function is used to ensure that data is available.
 *
 *  Returns data from RX software buffer.
 *
 *  \param usart_data       The USART_data_t struct instance.
 *
 *  \return         Received data.
 */
uint8_t USART_RXBuffer_GetByte(USART_data_t * usart_data)
{
	USART_Buffer_t * bufPtr;
	uint8_t ans;

	bufPtr = &usart_data->buffer;
	ans = (bufPtr->RX[bufPtr->RX_Tail]);

	/* Advance buffer tail. */
	bufPtr->RX_Tail = (bufPtr->RX_Tail + 1) & USART_RX_BUFFER_MASK;

	return ans;
}



/*! \brief RX Complete Interrupt Service Routine.
 *
 *  RX Complete Interrupt Service Routine.
 *  Stores received data in RX software buffer.
 *
 *  \param usart_data      The USART_data_t struct instance.
 */
bool USART_RXComplete(USART_data_t * usart_data)
{
	USART_Buffer_t * bufPtr;
	bool ans;
	
	bufPtr = &usart_data->buffer;
	/* Advance buffer head. */
	uint8_t tempRX_Head = (bufPtr->RX_Head + 1) & USART_RX_BUFFER_MASK;

	/* Check for overflow. */
	uint8_t tempRX_Tail = bufPtr->RX_Tail;
	uint8_t data = usart_data->usart->DATA;

	if (tempRX_Head == tempRX_Tail) {
	  	ans = false;
	}else{
		ans = true;
		usart_data->buffer.RX[usart_data->buffer.RX_Head] = data;
		usart_data->buffer.RX_Head = tempRX_Head;
	}
	return ans;
}


/*! \brief Receive complete interrupt service routine.
 *
 *  Receive complete interrupt service routine.
 *  Calls the common receive complete handler with pointer to the correct USART
 *  as argument.
 */
ISR(USARTD1_RXC_vect)
{
	USART_RXComplete(&BT_data);
}

/*! \brief Receive complete interrupt service routine.
 *
 *  Receive complete interrupt service routine.
 *  Calls the common receive complete handler with pointer to the correct USART
 *  as argument.
 */
ISR(USARTC0_RXC_vect)
{
	USART_RXComplete(&AUX_data);
}
