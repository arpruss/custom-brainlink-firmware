// Helper functions for use with the auxiliary PWM signals, see ext_pwm.c for more detail.

volatile int pwm_frequency = 0;

void set_pwm(void);
void turn_off_pwm0(void);
void turn_off_pwm1(void);
void set_pwm0(unsigned int duty);
void set_pwm1(unsigned int duty);