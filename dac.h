// Sets up the Xmega DAC for use with the two auxiliary outputs. See dac.c for more details.

void init_dac(void);
void set_dac0(uint8_t val);
void set_dac1(uint8_t val);
void disable_dac0(void);
void disable_dac1(void);