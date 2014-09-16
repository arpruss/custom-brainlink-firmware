/* Functions for manipulating the Xmega's DAC, for use with Brainlink's auxiliary ports */

#include "sine.c"
#if SINE_SIZE != WAVEFORM_SIZE
# error SINE_SIZE must equal WAVEFORM_SIZE
#endif

#define CPU_FREQUENCY 32000000ul

const uint8_t clockShifts[] = { 0, 1, 2, 3, 6, 8, 10 };

#define CLOCK_SHIFT_COUNT (sizeof clockShifts / sizeof *clockShifts)

// Set up the DAC to dual channel mode, 8 bit operation, VCC reference.
void init_dac() {
	DACB.CTRLB = DAC_CHSEL_DUAL_gc; // Dual channel mode
	DACB.CTRLC = DAC_REFSEL_AVCC_gc | 0x01;  // Set Analog voltage to reference, Left adjust to use just top 8 bits
	DACB.TIMCTRL = DAC_CONINTVAL_32CLK_gc | DAC_REFRESH_64CLK_gc; // 32 clock cycles per conversion, 64 per refresh
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
        disable_waveform0();
	DACB.CTRLA &= (~DAC_CH0EN_bm);
}

// Turn off the DAC on channel 1 to use the pin as regular I/O
void disable_dac1() {
        disable_waveform1();
	DACB.CTRLA &= (~DAC_CH1EN_bm);
}

void generate_waveform(uint8_t* waveform, char waveType, uint8_t dutyCycle, uint8_t amplitude, uint8_t waveShift) {
     uint8_t i;

     if (waveType == 'q') {
         for (i=0; i<dutyCycle ; i++)
             waveform[i] = amplitude;
         for (i=dutyCycle ; i < WAVEFORM_SIZE ; i++)
             waveform[i] = 0;
     }
     else {
         if (waveType == 's') {
             for (i=0; i<WAVEFORM_SIZE/2; i++)
                 waveform[i] = amplitude * (uint16_t)(uint8_t)pgm_read_byte_near(sine+i) / 255;
         }
         else {
             for (i=0; i<WAVEFORM_SIZE/2; i++)
                 waveform[i] = i * (uint16_t)amplitude / (WAVEFORM_SIZE/2);
         }

         for (i=WAVEFORM_SIZE/2 ; i<WAVEFORM_SIZE; i++)
             waveform[i] = amplitude - waveform[i-WAVEFORM_SIZE/2];
     }

     if (waveShift > 0) {
         uint8_t i;
         uint8_t j;
         uint8_t divider = (uint8_t)1 << waveShift;
         for (i=j=0 ; j < WAVEFORM_SIZE ; i++, j += divider) {
             waveform[i] = waveform[j];
         }
     }
}


/* returns 1 on success; 0 on frequency too high or too low for the length */
uint8_t play_arb_wave(uint8_t channel, uint8_t* waveform, uint8_t length, uint32_t frequency) {
    for (uint8_t clockShiftIndex = 0 ; clockShiftIndex < CLOCK_SHIFT_COUNT; clockShiftIndex++) {
         unsigned long period = (CPU_FREQUENCY >> clockShifts[clockShiftIndex]) / length / frequency;
         if (16 <= period && period < 65535u) {
              if (channel == 0)
                   play_arb_wave_dac0(waveform, length, clockShiftIndex, (uint16_t)period);
              else
                   play_arb_wave_dac1(waveform, length, clockShiftIndex, (uint16_t)period);
              return 1;
         }
    }

    return 0;
}

void play_wave_dac(uint8_t channel, char waveType, uint8_t dutyCycle, uint8_t amplitude, uint32_t frequency) {
     uint8_t waveShift = 0;
     uint8_t maxWaveShift = 3;

     if (waveType == 'q') {
         if (dutyCycle == WAVEFORM_SIZE / 2) {
              waveShift = 5; // two points only
              maxWaveShift = 5;
         }
         else if (dutyCycle == WAVEFORM_SIZE / 4 || dutyCycle == 3 * WAVEFORM_SIZE / 4) {
              waveShift = 4; // four points only
              maxWaveShift = 4;
         }
     }
     else if (waveType == 't') {
         maxWaveShift = 4;
     }

     disable_waveform(channel);

     /* try temporal rescalings of wave until it's within the xmega's ability to play it without too much distortion */
     for (; waveShift <= maxWaveShift ; waveShift++ ) {
          generate_waveform(waveform[channel], waveType, dutyCycle, amplitude, waveShift);
          if(play_arb_wave(channel, waveform[channel], WAVEFORM_SIZE >> waveShift, frequency))
              return;
     }
     err();
}

void play_arb_wave_dac0(const uint8_t* waveform, uint8_t len, uint8_t clockShiftIndex, uint16_t period) {
     turn_off_buzzer(); // just in case

     cli();

     DACB.TIMCTRL = DAC_CONINTVAL_32CLK_gc;

     DACB.CTRLC = DAC_REFSEL_AVCC_gc | DAC_LEFTADJ_bm;

     DMA.CH0.CTRLA = 0;

     TCD1.INTCTRLA = 0;
     TCD1.CTRLA = 1 + clockShiftIndex;
     TCD1.CTRLB = 0;
     TCD1.PER = period;
     TCD1.CNT = 0;
     EVSYS.CH1MUX = EVSYS_CHMUX_TCD1_OVF_gc;

     //DACB.EVCTRL = DAC_EVSEL_1_gc;

     //DACB.CTRLB |= DAC_CH0TRIG_bm;
     DACB.CTRLB &= ~DAC_CH0TRIG_bm;
     DACB.CTRLA |= DAC_CH0EN_bm | DAC_ENABLE_bm;

     DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_EVSYS_CH1_gc;

     DMA.CH0.ADDRCTRL = DMA_CH_DESTDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_SRCRELOAD_TRANSACTION_gc;
     DMA.CH0.TRFCNT = len;
     DMA.CH0.SRCADDR0  =(((uint16_t)waveform)) & 0xFF;
     DMA.CH0.SRCADDR1  =(((uint16_t)waveform)>>8) & 0xFF;
     DMA.CH0.SRCADDR2  =0;

     DMA.CH0.DESTADDR0 =(((uint16_t)(&DACB.CH0DATAH)))&0xFF;
     DMA.CH0.DESTADDR1 =(((uint16_t)(&DACB.CH0DATAH))>>8)&0xFF;
     DMA.CH0.DESTADDR2 =0;

     DMA.CH0.CTRLA = DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm | DMA_CH_ENABLE_bm;
     DMA.CTRL = DMA_ENABLE_bm;

     sei();
}

void play_arb_wave_dac1(const uint8_t* waveform, uint8_t len, uint8_t clockShiftIndex, uint16_t period) {
     set_ir0(); // just in case

     cli();

     DACB.TIMCTRL = DAC_CONINTVAL_32CLK_gc;

     DACB.CTRLC = DAC_REFSEL_AVCC_gc | DAC_LEFTADJ_bm;

     DMA.CH1.CTRLA = 0;

     TCC1.INTCTRLA = 0;
     TCC1.CTRLA = 1 + clockShiftIndex;
     TCC1.CTRLB = 0;
     TCC1.PER = period;
     TCC1.CNT = 0;
     EVSYS.CH2MUX = EVSYS_CHMUX_TCC1_OVF_gc;

     //DACB.EVCTRL = DAC_EVSEL_1_gc;

     //DACB.CTRLB |= DAC_CH0TRIG_bm;
     DACB.CTRLB &= ~DAC_CH1TRIG_bm;
     DACB.CTRLA |= DAC_CH1EN_bm | DAC_ENABLE_bm;

     DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_EVSYS_CH2_gc;

     DMA.CH1.ADDRCTRL = DMA_CH_DESTDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_SRCRELOAD_TRANSACTION_gc;
     DMA.CH1.TRFCNT = len;
     DMA.CH1.SRCADDR0  =(((uint16_t)waveform)) & 0xFF;
     DMA.CH1.SRCADDR1  =(((uint16_t)waveform)>>8) & 0xFF;
     DMA.CH1.SRCADDR2  =0;

     DMA.CH1.DESTADDR0 =(((uint16_t)(&DACB.CH1DATAH)))&0xFF;
     DMA.CH1.DESTADDR1 =(((uint16_t)(&DACB.CH1DATAH))>>8)&0xFF;
     DMA.CH1.DESTADDR2 =0;

     DMA.CH1.CTRLA = DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm | DMA_CH_ENABLE_bm;
     DMA.CTRL = DMA_ENABLE_bm;

     sei();
}

void disable_waveform(uint8_t channel) {
     if (channel == 0)
         disable_waveform0();
     else
         disable_waveform1();
}

void disable_waveform0() {
     DMA.CH0.CTRLA = 0;
     EVSYS.CH1MUX = 0;
     set_dac0(0);
}

void disable_waveform1() {
     DMA.CH1.CTRLA = 0;
     EVSYS.CH2MUX = 0;
     set_dac1(0);
}
