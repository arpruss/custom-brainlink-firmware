// Helper functions to control Brainlink's full color LED

static void init_led() {
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
void set_led(uint8_t red, uint8_t green, uint8_t blue) {
     // since the PWM duty cycle ranges from 0 to FFFF, but our
     // RGB values from 0 to FF, we put the same RGB value for each
     // of the two bytes of the duty cycle.
        red = (uint8_t)255 - red;
        green = (uint8_t)255 - green;
        blue = (uint8_t)255 - blue;
	TCD0.CCABUFH = red;
	TCD0.CCABUFL = red;
	TCD0.CCBBUFH = blue;
	TCD0.CCBBUFL = blue;
	TCD0.CCCBUFH = green;
	TCD0.CCCBUFL = green;
}
