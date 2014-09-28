/* Header file for functions controlling transmission of infrared signals. More detail in ir.c */

#define IR_OUT PIN0_bm

// Structure containing all of the data required to structure a signal
struct robotIRData {
	unsigned int frequency; // Carrier wave frequency
	char startUpPulseLength; // Number of start pulses
	unsigned int startUpPulse[16];  // The startup series of pulses for a given robot, up to 16 times available
	char bitEncoding;  // Whether bits are encoded by on-time, off-time, iRobot, alternating
	char numBits;   // number of bits in a message
	char numBytes;  // number of bytes in a message
	unsigned int highBitTime; // Time for a logical 1
	unsigned int lowBitTime;  // Time for a logical 0
	unsigned int offTime;     // Time in between bits (if applicable)
	char irBytes[192]; // Array holding signal data to send to the robot - probably only the first couple of bytes are necessary (1 byte for Robosapien, 2 for Prime-8) unless using raw encoding
	unsigned int repeatTime; // If the signal needs to be repeated, holds the time to repeat
	unsigned int repeatFlag; // set to 1 if signal needs to repeat, otherwise 0
}robotData;


int startUpCounter;
uint8_t bitCounter;
int onOffTracker;

// Initializes the IR transmitter
static void init_ir(void);
// Set the IR_LED to regular carrier frequency
static void set_ir50(void);
#ifdef COMPILE_UNUSED
// Set the IR_LED to full on - not used
static void set_ir100(void);
#endif
// Turn the IR LED off - not used
static void set_ir0(void);
// Sets up the IR carrier frequency
static void set_ir_carrier(int frequency);
// Starts the timer using the signal characteristics in robotIRData - the actual timer code is in ir.c
static void start_ir_timer(void);
// Stops the timer, ending the signal
static void stop_ir_timer(void);