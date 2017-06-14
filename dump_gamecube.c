static volatile PORT_t* port_from_pin(char pin) {
    if ('0' <= pin && pin <= '5')
        return &PORTA;
    else if (pin <= '7')
        return &PORTB;
    else  if (pin <= '9')
        return &PORTE;
    else
        return NULL;
}

static uint8_t bit_from_pin(char pin) {
    if ('0' <= pin && pin <= '5')
        return pin-'0';
    else if (pin == '6' || pin == '8')
        return 2;
    else 
        return 3;
}

uint8_t bitmap;
volatile PORT_t* port;
uint8_t buffer8[8];

static void gamecube_one(void) {
    port->OUTCLR = bitmap;
    _delay_us(5.*0.25);
    port->OUTSET = bitmap;
    _delay_us(5.*0.75);
}

static void gamecube_zeroes(uint8_t count) {
    while(count--) {
        port->OUTCLR = bitmap;
        _delay_us(5.*0.75);
        port->OUTSET = bitmap;
        _delay_us(5.*0.25);
    }
}

void dump_gamecube(char pin) {
    port = port_from_pin(pin);
    uint8_t bit = bit_from_pin(pin);
    bitmap = (uint8_t)1 << bit;
    
    if (port == NULL) {
        err();
        return;
    }
        
    
    if (pin == '6')
        disable_dac0();
    else if (pin == '7')
        disable_dac1();
    else if (pin == '8')
        turn_off_pwm0();
    else if (pin == '9')
        turn_off_pwm1();

    uint8_t timeout = 255;
    uint8_t curpos ;
    for(curpos=0; curpos<8; curpos++)
        buffer8[curpos] = 0;
    curpos = 0;
    uint8_t curbitmap = 1;

    port->DIRSET = bitmap;
    
    gamecube_zeroes(1);
    gamecube_one();
    gamecube_zeroes(12);
    gamecube_one();
    gamecube_one();
    gamecube_zeroes(6);
    gamecube_one();
    gamecube_zeroes(2);
    
    port->DIRCLR = bitmap;

    while(--timeout) {
        // wait for low
        if(!(port->IN & bitmap)) {            
            _delay_us(14./16.*2);
            if(port->IN & bitmap)
                buffer8[curpos] |= curbitmap;
            curbitmap <<= 1;
            if (curbitmap == 0) {
                curbitmap = 1;
                curpos++;
                if (curpos >= 8)
                    break;
            }
            timeout = 96;
            // wait for high
            while(--timeout && !(port->IN & bitmap));
            if (timeout==0) 
                break;
            timeout = 255;
        }
        asm("nop");
        asm("nop");
    }
    
    if (timeout==0) {
        err();
    }
    else {
        bt_putchar('G');
        bt_putchar('C');
        for(curpos=0;curpos<8;curpos++)
            bt_putchar(buffer8[curpos]);
    }
}
