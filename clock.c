
// Just sets up the Xmega clock to 32 MHz (defaults to 2 MHz without this)
static void init_clock()
{
	OSC.CTRL |= _BV(OSC_RC32MEN_bp);		// turn on 32MHz internal RC oscillator
	while(!(OSC.STATUS & OSC_RC32MRDY_bm));  	// wait for it to be ready
	CCP= CCP_IOREG_gc;			// allow modification of protected register
	CLK.CTRL |= CLK_SCLKSEL_RC32M_gc;	// change from 2MHz to 32MHz
}
