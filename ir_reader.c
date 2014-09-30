/* Used for capturing IR signals. */

// Initializes the IR capture capability, must be done each time a signal needs to be captured.
void init_ir_read() {
	
	ir_read_flag = 1; // Stays high until we return success or failure
	//divide clock by 64, so timer clock is 500 KHz
	TCE0.CTRLA = TC_CLKSEL_DIV64_gc; 

	//normal operation with capture compare port enabled
	TCE0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_NORMAL_gc;

	//pin change event from any pin triggers setting
	// Set up input capture when event channel 0 fires
	TCE0.CTRLD = TC_EVACT_CAPT_gc | TC_EVSEL_CH0_gc;
	// Clear the direction of the pin to make it an input
	PORTC.DIRCLR = PIN4_bm;
	// Invert the signal so we start with a rising edge
	// Defaults to sensing both edges, which is what we want
	PORTC.PIN4CTRL |= PORT_INVEN_bm;
	// Set event channel 0 to register events from port C pin 4
	EVSYS.CH0MUX = EVSYS_CHMUX_PORTC_PIN4_gc;

	//set PER to a value that will cut things off if no signal after 16 MS or so
	TCE0.PERBUF = 0x1F40;//TCE0.PERBUF = 0x7FFF;

	// Reset signal_count and time out
	signal_count = 0;
	ir_read_time_out = 0;

	// Enable interrupts on CCA and Overflow
	TCE0.INTCTRLA = TC_OVFINTLVL_HI_gc;
	TCE0.INTCTRLB = TC_CCAINTLVL_HI_gc;
	PMIC.CTRL |= PMIC_HILVLEN_bm;

	/* Currently used timers
	IR: TCC0 and TCC1
	Buzzer: TCD1
	LED: TCD0
	PWM: TCE0
	IR Reading: TCE0
	Waveform 0 (DACB, Channel 0): TCD1
	Waveform 1 (DACB, Channel 1): TCC1

         */

}

// All the fun stuff happens here - this interrupt is triggered when the input signal rises or falls
ISR(TCE0_CCA_vect) {
	TCE0.CNT = 0; // Reset the counter

	// If the number of edges is greater than 191, the signal is longer than we can store - so stop and return failure
	if(signal_count > 191) {
		disable_ir_read();
		ir_fail();
		return;
	}
	// Read the captured signal's time value (CCA register without the highest bit) into our array
	captured_signal[signal_count+1] = TCE0.CCAL;
	captured_signal[signal_count] = TCE0.CCAH&0b01111111;
	// Ignore spurious signals - software control for glitches from lighting, other remotes, etc - basically, ignores anything that's shorter than 200 microseconds and resets the counter
	if((captured_signal[signal_count+1] < 0x40) && (captured_signal[signal_count] == 0)) 
	{
		signal_count = 0;
	}
	// If the signal count increases past a typical "start-up" section, tighten the spacing between
	// rising and falling edges to 8 ms to prevent multiple consecutive reads of the same signal
	if((signal_count > 16) & (captured_signal[signal_count] > 0x10)) {
		disable_ir_read();
		dump_data(); // Prints all of the data over serial
		return;	
	}
	signal_count+=2; // Count by 2s because the signal time data takes two bytes
}

// If we get to an overflow (roughly every 16 ms if no captures occur), then either no signal has been seen yet, or one has been seen and has ended
ISR(TCE0_OVF_vect) {
	// If the signal_count is non zero, we've seen a signal, so dump it over serial
	if(signal_count != 0) {
		disable_ir_read();
		dump_data();
	}
	// Otherwise, increment the time out counter - if we overflow 250 or more times, report failure to read a signal
	else {
		ir_read_time_out++;
		if(ir_read_time_out > 250) {
			disable_ir_read();
			ir_fail();
		}
	}
}

// Just prints ERR to serial to show that signal was not captured
void ir_fail()
{
        err();
	ir_read_flag = 0;
}

// Disables the interrupts/event system used to capture IR signal
void disable_ir_read() 
{
	// Turn off interrupts:
	TCE0.INTCTRLA = TC_OVFINTLVL_OFF_gc;
	TCE0.INTCTRLB = TC_CCAINTLVL_OFF_gc; 
	// Turn off event system
	EVSYS.CH0MUX  = 0;
	TCE0.CTRLD = 0;
	//Turn off Timer-counter
	TCE0.CTRLB = 0;
	TCE0.CTRLA = 0;
}

// Dumps the captured data over serial
void dump_data()
{
	// If signal_count is 2 or 4, we picked up some spurious signal, so don't print those out, instead, send ir_fail()
	if(signal_count > 4) {
		uart_putchar(&BT_USART, signal_count-1); // print the length of the signal including the signal length byte. Does not include first two bytes
		// First two bytes don't get considered part of the signal because they represent a random value (random time to the time of the first edge)
		for(uint8_t i = 2; i < signal_count; i++)
		{
			uart_putchar(&BT_USART, captured_signal[i]);
		}
		ir_read_flag = 0;
	}
	else {
		ir_fail();
	}
}

// Write data takes the captured signal array and writes it into eeprom
void write_data_to_eeprom(int position) 
{
	int pages = (signal_count-1)/32; // The first two bytes of the signal are invalid, but we need an extra byte to hold the length of the signal
	int remainder = (signal_count-1)%32;
	int array_counter = 2;
	int j = 0;
	
	// Write the length of the data - signal_count-2, plus 1 byte to hold the length of the signal
	// Uses the EEPROM driver by Atmel, included in the source package.
	EEPROM_WriteByte(position*6, j, signal_count-1);
	j++;
	for(int i = 0; i < pages; i++) {
		while(j < EEPROM_PAGESIZE) {
			EEPROM_WriteByte(position*6 + i, j, captured_signal[array_counter]);
			array_counter++;
			j++;
		}
		j=0;
	}
	
	while(j < remainder) {
		EEPROM_WriteByte(position*6 + pages, j, captured_signal[array_counter]);
		array_counter++;
		j++;
	}
}	

// Reads the data in a given EEPROM position into the robotData structure, from where it can be used to re-transmit the signal
void read_data_from_eeprom(int position)
{
	int signal_length = EEPROM_ReadByte(position*6, 0);
	int pages = signal_length/32;
	int remainder = signal_length%32;
	int array_counter = 0;
	int j = 1;
	
	for(int i = 0; i < pages; i++) {
		while(j < EEPROM_PAGESIZE) {
			robotData.irBytes[array_counter] = EEPROM_ReadByte(position*6 + i, j);
			array_counter++;
			j++;
		}
		j = 0;
	}
	
	while(j < remainder) {
		robotData.irBytes[array_counter] = EEPROM_ReadByte(position*6 + pages, j);
		array_counter++;
		j++;
	}
	robotData.numBits = signal_length-1;
}	

// Sends the data in EEPROM over bluetooth to the host
void print_data_from_eeprom(int position)
{
	int signal_length = EEPROM_ReadByte(position*6, 0);
	int pages = signal_length/32;
	int remainder = signal_length%32;
	int j = 1;
	
	uart_putchar(&BT_USART, signal_length);
	
	for(int i = 0; i < pages; i++) {
		while(j < EEPROM_PAGESIZE) {
			uart_putchar(&BT_USART, EEPROM_ReadByte(position*6 + i, j));
			j++;
		}
		j = 0;
	}
	
	while(j < remainder) {
		uart_putchar(&BT_USART, EEPROM_ReadByte(position*6 + pages, j));
		j++;
	}
}

// Reads raw data from the host into the robotData structure, where it can be used to re-transmit the data.
int read_data_from_serial()
{
	int signal_length = bt_getchar_timeout_echo();
	int temp = 0;

	for(int i = 0; i < signal_length-1; i++) {
		temp = bt_getchar_timeout_echo();
		if(temp == 256) {
			return 0;
		}
		else {
			robotData.irBytes[i] = (char)temp;
		}
	}

	robotData.numBits = signal_length-1;
	return 1;
}
