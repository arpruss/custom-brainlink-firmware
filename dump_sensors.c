static void put_hex_nibble(uint8_t nibble) {
    if (nibble<10)
        bt_putchar(nibble+'0');
    else
        bt_putchar(nibble+('A'-10));
}

void dump_sensors(uint16_t digital, uint8_t analog) {
    uint8_t pin;
    uint8_t value;
    while(1) {
        if (USART_RXBufferData_Available(&BT_data) && '*' == USART_RXBuffer_GetByte(&BT_data))
            return;
    }
    bt_putchar('a');
    for (pin=0; pin<10; pin++) 
        if (digital & (1<<pin))
            bt_putchar('0'+read_input(pin));
    for (pin=0; pin<6; pin++) 
        if (analog & (1<<pin)) {
            value = read_analog(pgm_read_byte_near(muxPosPins+pin), 0);
            put_hex_nibble(value>>4);
            put_hex_nibble(value&0xF);
        }
}
