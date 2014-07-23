/** Configures and sets the buzzer */


// Configures the buzzer
void init_buzzer() {
	// Set frequency maximum to 62,500 - clockspeed/256
	TCD1.CTRLA = TC_CLKSEL_DIV256_gc;
	// Set counter to frequency mode, this lets us vary frequency and keeps duty cycle at 50%
	TCD1.CTRLB = TC_WGMODE_FRQ_gc;
	// Set pin to output
	PORTD.DIRSET |= PIN4_bm;
	PORTD.OUTCLR |= PIN4_bm;
}

// Set buzzer enables the waveform and sets it to a given frequency
void set_buzzer(int frequency) {
	// Enable waveform output
	TCD1.CTRLB |= TC1_CCAEN_bm;
	// Set the frequency
	cli();
	TCD1.CCA = frequency;
	sei();
}

// Disables the buzzer, and if the output is on, turns it off
void turn_off_buzzer() {
	// Disable the Waveform output
	TCD1.CTRLB &= (~TC1_CCAEN_bm);
	// Turn off the pin just in case
	PORTD.OUTCLR |= PIN4_bm;
}
	