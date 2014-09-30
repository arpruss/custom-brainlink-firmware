/* Some code to use for the accelerometer. Notice that the code uses Xmegas TWI driver
   and includes some function calls to the TWI driver. Your code may need to include this driver */

#include "accelerometer.h"


#define ACCEL_I2C_ADDR 0x4C    	// 0x98 >> 1;  << 1 and OR with 0x01 for i2c read address
#define CPU_SPEED   32000000
#define BAUDRATE	100000
#define TWI_BAUDSETTING TWI_BAUD(CPU_SPEED, BAUDRATE)

TWI_Master_t twiMaster;

AccelData _acc;


#ifdef COMPILE_UNUSED
// Function to return the data - must call updateAccel() first
AccelData getAccelData()
{
	return _acc;
}

// Tells you if new data is ready
int newAccelData()
{
	return (twiMaster.result == TWIM_RESULT_OK && twiMaster.status == TWIM_STATUS_READY);
}
#endif

static void sendAccel(uint16_t value) {
    TWI_MasterWriteRead(&twiMaster, ACCEL_I2C_ADDR, (uint8_t*)&value, 2, 0);
    while (twiMaster.status != TWIM_STATUS_READY);
}

// this is currently non-blocking. Blocking was attempted (commented out) but seemed to block forever.
void updateAccel()
{
//	if(twiMaster.result == TWIM_RESULT_OK && twiMaster.status == TWIM_STATUS_READY)
//	{ 
		//char oldstatus = _acc.status & ACC_O_MASK;
		uint8_t buf = 0x00;// read 4 bytes, starting at 0x00 (x,y,z,status)
		TWI_MasterWriteRead(&twiMaster, ACCEL_I2C_ADDR, &buf, 1, 4);
		
		while (twiMaster.status != TWIM_STATUS_READY);
		_acc.x = twiMaster.readData[0];
		_acc.y = twiMaster.readData[1];
		_acc.z = twiMaster.readData[2];
		_acc.status = twiMaster.readData[3];
		
		twiMaster.result = TWIM_RESULT_UNKNOWN;
		
//	}
}

// package two bytes as a little-endian word and send them to the acceleromete
#define SEND_ACCEL(b1,b2) sendAccel( (b1) | ((b2)<<8) )

// Initializes the accelerometer
// Doesn't *really* need to be blocking, just checked for success
void initAccel()
{
	cli();

	/* Initialize TWI master. */
	TWI_MasterInit(&twiMaster,
	               &TWIE,
	               TWI_MASTER_INTLVL_LO_gc,
	               TWI_BAUDSETTING);

	/* Enable PMIC interrupt level low. */
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
	sei();

	// set mode to STANDBY if it isn't already (can't update registers in ACTIVE mode!)
	SEND_ACCEL(0x07, 0b01010000); // set interrupt to push-pull, mode to standby (mode and Ton are both 0), auto-sleep to enable
	// set sleep mode
        SEND_ACCEL(0x05, 0x00); // no sleep
	// set interrupts
        SEND_ACCEL(0x06, 0b11100100);  // set interrupts to occur with a tap or shake (PDINT, SHINTX/Y/Z)
        // set filter rate
        SEND_ACCEL(0x08, 0xE0);  // Set filter rate to 8 samples, set number of samples to 120 in waking state, 32 in sleeping (hopefully we avoid sleep)
	// set tap detection
	SEND_ACCEL(0x09, 0b00001000);  	// 0x09	Pulse detection -> +- 8 counts, all axes
        // set tap debounce
	SEND_ACCEL(0x0A, 0x10);	// Tap debounce ->	16 detections
	// enable the accelerometer
        SEND_ACCEL(0x07, 0b01010001);
}

/*! TWIE Master Interrupt vector. */
ISR(TWIE_TWIM_vect)
{
	TWI_MasterInterruptHandler(&twiMaster);
}