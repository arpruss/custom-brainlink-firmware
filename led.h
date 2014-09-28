// Helper functions to control Brainlink's full color LED, more in LED.c

// Set the pins and port of the LED
#define REDLED    PIN0_bm
#define GREENLED  PIN2_bm
#define BLUELED   PIN1_bm
#define LEDPORT   PORTD

void init_led(void);
void set_led(uint8_t red, uint8_t green, uint8_t blue);