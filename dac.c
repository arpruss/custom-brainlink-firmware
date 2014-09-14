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
        disable_waveform();
	DACB.CTRLA &= (~DAC_CH0EN_bm);
}

// Turn off the DAC on channel 1 to use the pin as regular I/O
void disable_dac1() {
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

void play_wave_dac0(char waveType, uint8_t dutyCycle, uint8_t amplitude, uint32_t frequency) {
     for (uint8_t waveShift = 0 ; waveShift < 4 ; waveShift++ ) {
          for (uint8_t clockShiftIndex = 0 ; clockShiftIndex < CLOCK_SHIFT_COUNT; clockShiftIndex++) {
               unsigned long period = (CPU_FREQUENCY >> clockShifts[clockShiftIndex]) / (WAVEFORM_SIZE >> waveShift) / frequency;
               if (32 <= period && period < 65535u) {
//                    bt_putchar(period>>8);
//                    bt_putchar(period&0xff);
//                    bt_putchar(clockShiftIndex);
//                    bt_putchar(waveShift);
                    generate_waveform(waveform, waveType, dutyCycle, amplitude, waveShift);
                    play_arb_wave_dac0(waveform, WAVEFORM_SIZE >> waveShift, clockShiftIndex, (uint16_t)period);
                    return;
               }
          }
     }
     err();
}

void play_arb_wave_dac0(const uint8_t* waveform, uint8_t len, uint8_t clockShiftIndex, uint16_t period) {
     turn_off_buzzer(); // just in case
     cli();

//                    bt_putchar(period>>8);
//                    bt_putchar(period&0xff);
//                    bt_putchar(clockShiftIndex);
//                    bt_putchar(len);

     DACB.TIMCTRL = DAC_CONINTVAL_32CLK_gc;

     DACB.CTRLC = DAC_REFSEL_AVCC_gc | DAC_LEFTADJ_bm;

     DMA.CH0.CTRLA = 0;

     TCD1.CTRLA = 1 + clockShiftIndex;
     TCD1.CTRLB = 0;
     TCD1.PER = period;
     EVSYS.CH1MUX = EVSYS_CHMUX_TCD1_OVF_gc;

     DACB.EVCTRL = DAC_EVSEL_1_gc;

     DACB.CTRLB |= DAC_CH0TRIG_bm;
     DACB.CTRLA |= DAC_CH0EN_bm | DAC_ENABLE_bm;

     DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_DACB_CH0_gc;

     DMA.CH0.ADDRCTRL = DMA_CH_DESTDIR_FIXED_gc | DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_SRCRELOAD_TRANSACTION_gc;
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

void disable_waveform() {
     DMA.CH0.CTRLA = 0;
     EVSYS.CH1MUX = 0;
     TCD1.CTRLA = 0;
}
