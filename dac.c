/* Functions for manipulating the Xmega's DAC, for use with Brainlink's auxiliary ports */

#include "sine.c"

// Set up the DAC to dual channel mode, 8 bit operation, VCC reference.
void init_dac() {
	DACB.CTRLB = DAC_CHSEL_DUAL_gc; // Dual channel mode
	DACB.CTRLC = DAC_REFSEL_AVCC_gc | 0x01;  // Set Analog voltage to reference, Left adjust to use just top 8 bits
	DACB.TIMCTRL = DAC_CONINTVAL_32CLK_gc | DAC_REFRESH_64CLK_gc; // 64 clock cycles per conversion, 256 per refresh
	DACB.CTRLA = DAC_ENABLE_bm;	// Enable channels 0 and 1 and enable DAC
	set_dac0(0); // Set both DACs to 0
	set_dac1(0);
}

// Sets DAC Ch0
void set_dac0(uint8_t val) {
	// enable the DAC
	DACB.CTRLA |= DAC_CH0EN_bm;

	// Wait for data register to be open
	while(!(DACB.STATUS & DAC_CH0DRE_bm));

	DACB.CH0DATAH = val;
}

// Sets DAC Ch1
void set_dac1(uint8_t val) {
	// enable the DAC
	DACB.CTRLA |= DAC_CH1EN_bm;

	// Wait for data register empty
	while(!(DACB.STATUS & DAC_CH1DRE_bm));

	DACB.CH1DATAH = val;
}

// Turn off the DAC on channel 0 to use the pin as regular I/O
void disable_dac0() {
	DACB.CTRLA &= (~DAC_CH0EN_bm);
}

// Turn off the DAC on channel 1 to use the pin as regular I/O
void disable_dac1() {
	DACB.CTRLA &= (~DAC_CH1EN_bm);
}

void play_wave_dac(uint8_t channel, int8_t* waveform, int len, unsigned long int freq) {
  
}

