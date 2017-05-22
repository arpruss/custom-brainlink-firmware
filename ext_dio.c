// Helper functions for setting outputs and reading inputs on the auxiliary digital I/O.

//turns any of the external DIO into an ouput and sets its value
void set_output(char port, char value) {
	switch(port) {
		case '0':
			PORTA.DIRSET = PIN0_bm;
			if(value) {
				PORTA.OUTSET = PIN0_bm;
			}
			else {
				PORTA.OUTCLR = PIN0_bm;
			}
			break;
		case '1':
			PORTA.DIRSET = PIN1_bm;
			if(value) {
				PORTA.OUTSET = PIN1_bm;
			}
			else {
				PORTA.OUTCLR = PIN1_bm;
			}
			break;
		case '2':
			PORTA.DIRSET = PIN2_bm;
			if(value) {
				PORTA.OUTSET = PIN2_bm;
			}
			else {
				PORTA.OUTCLR = PIN2_bm;
			}
			break;
		case '3':
			PORTA.DIRSET = PIN3_bm;
			if(value) {
				PORTA.OUTSET = PIN3_bm;
			}
			else {
				PORTA.OUTCLR = PIN3_bm;
			}
			break;
		case '4':
			PORTA.DIRSET = PIN4_bm;
			if(value) {
				PORTA.OUTSET = PIN4_bm;
			}
			else {
				PORTA.OUTCLR = PIN4_bm;
			}
			break;
		case '5':
			PORTA.DIRSET = PIN5_bm;
			if(value) {
				PORTA.OUTSET = PIN5_bm;
			}
			else {
				PORTA.OUTCLR = PIN5_bm;
			}
			break;
		case '6':
			// Disable the DAC ch0 in case it's on
			disable_dac0();
			PORTB.DIRSET = PIN2_bm;
			if(value) {
				PORTB.OUTSET = PIN2_bm;
			}
			else {
				PORTB.OUTCLR = PIN2_bm;
			}
			break;
		case '7':
			// Disable the DAC ch1 in case it's on
			disable_dac1();
			PORTB.DIRSET = PIN3_bm;
			if(value) {
				PORTB.OUTSET = PIN3_bm;
			}
			else {
				PORTB.OUTCLR = PIN3_bm;
			}
			break;
		case '8':
			// Disable PWM ch0 in case it's on
			turn_off_pwm0();
			PORTE.DIRSET = PIN2_bm;
			if(value) {
				PORTE.OUTSET = PIN2_bm;
			}
			else {
				PORTE.OUTCLR = PIN2_bm;
			}
			break;
		case '9':
			// Disable PWM ch1 in case it's on
			turn_off_pwm1();
			PORTE.DIRSET = PIN3_bm;
			if(value) {
				PORTE.OUTSET = PIN3_bm;
			}
			else {
				PORTE.OUTCLR = PIN3_bm;
			}
			break;
		default:
			break;
	}

}

// Turns any of the external DIO into an input and reads the value
char read_input(char port) {
	switch(port) {
		case '0':
			PORTA.DIRCLR = PIN0_bm;
			return (PORTA.IN & PIN0_bm)>>0;
			break;
		case '1':
			PORTA.DIRCLR = PIN1_bm;
			return (PORTA.IN & PIN1_bm)>>1;
			break;
		case '2':
			PORTA.DIRCLR= PIN2_bm;
			return (PORTA.IN & PIN2_bm)>>2;
			break;
		case '3':
			PORTA.DIRCLR= PIN3_bm;
			return (PORTA.IN & PIN3_bm)>>3;
			break;
		case '4':
			PORTA.DIRCLR = PIN4_bm;
			return (PORTA.IN & PIN4_bm)>>4;
			break;
		case '5':
			PORTA.DIRCLR = PIN5_bm;
			return (PORTA.IN & PIN5_bm)>>5;
			break;
		case '6':
			// Disable DAC ch0 in case it's on
			disable_dac0();
			PORTB.DIRCLR = PIN2_bm;
			return (PORTB.IN & PIN2_bm)>>2;
			break;
		case '7':
			// Disable DAC ch0 in case it's on
			disable_dac1();
			PORTB.DIRCLR = PIN3_bm;
			return (PORTB.IN & PIN3_bm)>>3;
			break;
		case '8':
			// Disable PWM ch0 in case it's on
			turn_off_pwm0();
			PORTE.DIRCLR = PIN2_bm;
			return (PORTE.IN & PIN2_bm)>>2;
			break;
		case '9':
			// Disable PWM ch1 in case it's on
			turn_off_pwm1();
			PORTE.DIRCLR = PIN3_bm;
			return (PORTE.IN & PIN3_bm)>>3;
			break;
		default:
			return -48;
			break;
	}
}
    
// mode: 'u': pull-up, 'd': pull-down, '0': float
void set_pull_mode(uint8_t port, char mode) {
    uint8_t control;
    if (mode == 'u')
        control = PORT_OPC_PULLUP_gc;
    else if (mode == 'd')
        control = PORT_OPC_PULLDOWN_gc;
    else
        control = 0;
    
	switch(port) {
		case '0':
			PORTA.PIN0CTRL = control;
			break;
		case '1':
			PORTA.PIN1CTRL = control;
			break;
		case '2':
			PORTA.PIN2CTRL = control;
			break;
		case '3':
			PORTA.PIN3CTRL = control;
			break;
		case '4':
			PORTA.PIN4CTRL = control;
			break;
		case '5':
			PORTA.PIN5CTRL = control;
			break;
		case '6':
			PORTA.PIN6CTRL = control;
			break;
		case '7':
			PORTA.PIN7CTRL = control;
			break;
		case '8':
			turn_off_pwm0();
			PORTE.PIN2CTRL = control;
			break;
		case '9':
			turn_off_pwm1();
			PORTE.PIN3CTRL = control;
			break;
    }
}

