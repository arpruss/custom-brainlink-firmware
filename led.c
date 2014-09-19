// Helper functions to control Brainlink's full color LED

void init_led() {
	// Set clock to 4 MHz
	TCD0.CTRLA = TC_CLKSEL_DIV8_gc;
	
	// Turn on single slope pwm waveform generation on CCA, B, and C (LED pins)
	TCD0.CTRLB = (TC0_CCCEN_bm | TC0_CCBEN_bm | TC0_CCAEN_bm) | TC_WGMODE_SS_gc;

	// Set period to 0xFFFF
	TCD0.PERBUF = 0xFFFF;

	// Set LED pins to output
	LEDPORT.DIRSET |= REDLED | GREENLED | BLUELED;
	// Set the LED to 0, just in case
	set_led(0,0,0);
}

// Update the capture compare registers with new led values
// Invert values since our LED is common anode
void set_led(int red, int green, int blue) {
	// Use just the high byte of the CC buffer, since we only desire 8 bit color
	TCD0.CCABUFH = 255-red;
	TCD0.CCBBUFH = 255-blue;
	TCD0.CCCBUFH = 255-green;
}
