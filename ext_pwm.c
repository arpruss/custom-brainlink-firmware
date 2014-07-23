// Helper functions to set the auxiliary PWM signals.

// Sets up PWM to a given frequency (both channels must share the same frequency)
void set_pwm() {

	// Set clock to 32 MHz
	TCE0.CTRLA = TC_CLKSEL_DIV1_gc;
	
	// Turn on single slope pwm waveform generation
	TCE0.CTRLB = TC_WGMODE_SS_gc;
	// Turn off other registers in case they were set by IR read
	TCE0.CTRLD = 0x00;
	// Set frequency of the PWM signal
	TCE0.PERBUF = pwm_frequency;

}

// Turns off the waveform on channel 0, so regular I/O can be used
void turn_off_pwm0() {
	TCE0.CTRLB &= (~TC0_CCCEN_bm);
}

// Turns off the waveform on channel 1, so regular I/O can be used
void turn_off_pwm1() {
	TCE0.CTRLB &= (~TC0_CCDEN_bm);
}

// Sets up a waveform at a given duty cycle. Duty cycle variable depends on frequency (see:
// http://www.brainlinksystem.com/brainlink-hardware-description#commands
void set_pwm0(unsigned int duty) {
	TCE0.CTRLB |= TC0_CCCEN_bm;
	PORTE.DIRSET = PIN2_bm;
	TCE0.CCC = duty;
}

// Sets up a waveform at a given duty cycle. Duty cycle variable depends on frequency (see:
// http://www.brainlinksystem.com/brainlink-hardware-description#commands
void set_pwm1(unsigned int duty) {
	TCE0.CTRLB |= TC0_CCDEN_bm;
	PORTE.DIRSET = PIN3_bm;
	TCE0.CCD = duty;
}
