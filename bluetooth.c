/** For use with Brainlink's RN-42 module. Just includes a few helper functions, most of the serial comm stuff is in uart.c. */


// Resets the module, and then initializes the Xmega's UART that is attached to the module
void init_BT() {
	// Set the pins
	BT_RESET_PORT.DIRSET = BT_RESET_PIN;
	BT_DEFAULT_RESET_PORT.DIRSET = BT_DEFAULT_RESET_PIN;

	// Reset once
	BT_reset();
	BT_DEFAULT_RESET_PORT.OUTCLR = BT_DEFAULT_RESET_PIN;	

	// Set up uart connected to bluetooth
	init_bt_uart();

}

// Restores factory default settings on the module, not currently used
#ifdef COMPILE_UNUSED
void BT_default_reset() {
	// Turns the pin on, and then does the reset by toggling the pin three times
	BT_DEFAULT_RESET_PORT.OUTSET = BT_DEFAULT_RESET_PIN;
	_delay_ms(10);
	for(int i = 0; i < 6; i++) {
		BT_DEFAULT_RESET_PORT.OUTTGL = BT_DEFAULT_RESET_PIN;
		_delay_ms(10);
	}
	BT_DEFAULT_RESET_PORT.OUTCLR = BT_DEFAULT_RESET_PIN;

}
#endif

// Resets the module
void BT_reset() {

	BT_RESET_PORT.OUTCLR = BT_RESET_PIN;
	_delay_ms(10);
	BT_RESET_PORT.OUTSET = BT_RESET_PIN;
}
