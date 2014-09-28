
#define BT_RESET_PIN           PIN1_bm  // PORTC
#define BT_RESET_PORT          PORTC
#define BT_DEFAULT_RESET_PIN   PIN5_bm  // PORTD
#define BT_DEFAULT_RESET_PORT  PORTD

void init_BT(void);
void BT_reset(void);