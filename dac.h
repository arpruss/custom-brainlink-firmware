// Sets up the Xmega DAC for use with the two auxiliary outputs. See dac.c for more details.

#define WAVEFORM_SIZE 64
uint8_t waveform[2][WAVEFORM_SIZE];

static void init_dac(void);
void set_dac0(uint8_t val);
void set_dac1(uint8_t val);
void disable_dac0(void);
void disable_dac1(void);
uint8_t play_arb_wave(uint8_t channel, uint8_t* waveform, uint8_t length, uint32_t frequency);
void play_arb_wave_dac0(const uint8_t* waveform, uint8_t len, uint8_t clockShiftIndex, uint16_t period);
void play_arb_wave_dac1(const uint8_t* waveform, uint8_t len, uint8_t clockShiftIndex, uint16_t period);
static void play_wave_dac(uint8_t channel, char waveType, uint8_t dutyCycle, uint8_t amplitude, uint32_t frequency);
void generate_waveform(uint8_t* waveform, char waveType, uint8_t dutyCycle, uint8_t amplitude, uint8_t waveShift);
static void disable_waveform0(void);
static void disable_waveform1(void);
static void disable_waveform(uint8_t channel);