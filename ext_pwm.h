// Helper functions for use with the auxiliary PWM signals, see ext_pwm.c for more detail.

volatile int pwm_frequency = 0;

static void set_pwm(void);
static void turn_off_pwm0(void);
static void turn_off_pwm1(void);
static void set_pwm0(unsigned int duty);
static void set_pwm1(unsigned int duty);