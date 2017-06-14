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

static uint8_t get_gamecube_data(uint8_t rumble) {
    uint8_t timeout = 255;
    uint8_t curpos;
    
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
    if (rumble)
        gamecube_one();
    else
        gamecube_zeroes(1);
    gamecube_one(); // stop bit
    
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
                if (curpos >= 8) {
                    bt_putchar('G');
                    for(curpos=0;curpos<8;curpos++)
                        bt_putchar(buffer8[curpos]);
                    return 1;
                }
            }
            timeout = 96;
            // wait for high
            while(--timeout && !(port->IN & bitmap));
            if (timeout==0) {
                return 0;
            }
            timeout = 255;
        }
        asm("nop");
        asm("nop");
    }
    
    return 0;
}

void dump_gamecube(char pin, uint8_t stream, uint8_t rumble) {
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
    
    if (!stream) {
        if (! get_gamecube_data(rumble)) {
            err();
        }
    }
    else {
        while(1) {
            if (USART_RXBufferData_Available(&BT_data)) {
                char c = USART_RXBuffer_GetByte(&BT_data);
                if (c=='*')
                    return;
                else if (c=='r')
                    rumble = 0;
                else if (c=='R')
                    rumble = 1;
            }
            get_gamecube_data(rumble);
            _delay_ms(5);
        }
    }
}
