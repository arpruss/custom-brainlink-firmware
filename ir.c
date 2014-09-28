/* Source file for functions controlling transmission of infrared signals. */

// Initializes the timers used to control the IR carrier and the IR signal
void init_ir() {
        disable_waveform1();

	// Set timer 0 clock to 32 MHz
	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;

	// Turn on single slope pwm waveform generation on CCA (IR Pin)
	TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_SS_gc;

	// Set period to 0x0349 for 38000 Hz operation
	// This easily adjusts the frequency of the carrier wave
	TCC0.PERBUF = 0x0349;

	// Set IR pin to output
	PORTC.DIRSET = IR_OUT;

	// Set timer to count at 500 KHz - this timer is used to construct the IR signal
	TCC1.CTRLA = TC_CLKSEL_DIV64_gc;
	TCC1.INTCTRLA = TC_OVFINTLVL_OFF_gc;
}

// Set the IR_LED to regular carrier frequency
void set_ir50() {
	cli();
	TCC0.CCA = (TCC0.PER/2);
	sei();
}


#ifdef COMPILE_UNUSED
// Set the IR_LED to full on
void set_ir100() {
	cli();
	TCC0.CCA = TCC0.PER;
	sei();
}
#endif

// Turn the IR LED off
void set_ir0() {
	cli();
	TCC0.CCA = 0x0000;
	sei();
}

// Sets the frequency of the IR carrier
void set_ir_carrier(int frequency) {

	cli();
	TCC0.PERBUF = frequency;
	sei();
}

// Starts timer 1, used to construct the IR signal
void start_ir_timer() {
    // Reset counters
	startUpCounter = 0;
	onOffTracker = 0;
	bitCounter = 0;

	cli();
	TCC1.CNT = 0x0000; // Set timer 1 counter to 0
 	sei();
	// Enable interrupts on timer
	TCC1.INTCTRLA = TC_OVFINTLVL_HI_gc;
	PMIC.CTRL |= PMIC_HILVLEN_bm;
}

// Stops the timer by turning off the interrupt
void stop_ir_timer() {
	TCC1.INTCTRLA = TC_OVFINTLVL_OFF_gc;
	// Turn off IR LED just in case it was left on
	set_ir0();
}


// The timer overflow on TCC1 - this is where most of the action is.
ISR(TCC1_OVF_vect) {

	// The following if-else basically alternates the signal each time the interrupt is triggered
	// If the signal should be low right now, set it that way
	if(onOffTracker == 1) {
		set_ir0();
		onOffTracker = 0;
	}
	// Else turn on the signal using a 50% duty cycle (50% at carrier frequency)
	else {
		set_ir50();
		onOffTracker = 1;
	}

	// Checks if we've transmitted all the signal's bits. If so, turns off IR or sets the timer to interrupt again in time specified by robotData.repeatTime
	if((bitCounter >= robotData.numBits) && ((onOffTracker==0)||(robotData.bitEncoding == 0x03))) {
		// iRobot's encoding is funky in the last bit and needs to be turned off right away
		if(robotData.bitEncoding == 0x03) {
			set_ir0();
		}
		// If repeatFlag = 0, stop the IR timer, effectively ending transmission of the signal
		if(!robotData.repeatFlag) {
			stop_ir_timer();
		}
		// Otherwise, set the timer to interrupt again in robotData.repeatTime and reset all counters. At that time it will repeat the signal.
		else {
			startUpCounter = 0;
			onOffTracker = 0;
			bitCounter = 0;
			TCC1.PER = robotData.repeatTime;
		}
	}
	// If we're still transmitting, do the following
	else {
		// If we're still in the "start pulse" phase of the signal, then set the timer to interrupt again in the time specified by startUpPulse[startUpCounter]
		if(startUpCounter < robotData.startUpPulseLength)
		{
			TCC1.PER = robotData.startUpPulse[startUpCounter];
			startUpCounter++; // increment counter
		}
		// Else, send data according to encoding format
		else {
			// If format is alternating (0):
			if(robotData.bitEncoding == 0x00) {
				// If the bit should be a logical 1, interrupt again in time specified by highBitTime
				if(robotData.irBytes[bitCounter/8] & (0x01<<(7-(bitCounter%8)))) {
					TCC1.PER = robotData.highBitTime;
				}
				// Else, use lowBitTime
				else {
					TCC1.PER = robotData.lowBitTime;
				}
				bitCounter++;
			}
			// If format is up-time (1):
			else if(robotData.bitEncoding == 0x01) {
				// If this tracker is a 1, then we just set IR on
				// If the tracker is showing a 1, then the next pulse is used to encode a bit by pulse length
				if(onOffTracker == 1) {
					// If the bit will be a logical 1, use highBitTime, else, use lowBitTime
					if(robotData.irBytes[bitCounter/8] & (0x01<<(7-(bitCounter%8)))) {
						TCC1.PER = robotData.highBitTime;
					}
					else {
						TCC1.PER = robotData.lowBitTime;
					}
					bitCounter++;
				}
				// If the onOffTracker is 0, the next time is used to space bits, so use "offTime"
				else {
					TCC1.PER = robotData.offTime;
				}
			}
			// If format is down-time (2):
			else if(robotData.bitEncoding == 0x02) {
				// If this tracker is a 0, then we just set IR off
				// If the tracker is showing a 0, then the next signal off period is used to encode a bit by time
				if(onOffTracker == 0) {
				/// If the bit will be a logical 1, use highBitTime, else, use lowBitTime
					if(robotData.irBytes[bitCounter/8] & (0x01<<(7-(bitCounter%8)))) {
						TCC1.PER = robotData.highBitTime;
					}
					else {
						TCC1.PER = robotData.lowBitTime;
					}
				}
				// If the onOffTracker is 1, the next pulse time is used to space bits, so use "offTime"
				else {
					TCC1.PER = robotData.offTime;
					bitCounter++;

				}
			}
			// If iRobot encoding (3):
			else if(robotData.bitEncoding == 0x03) {
				// If this tracker is a 0, then we just set IR off
				if(onOffTracker == 0) {
					if(robotData.irBytes[bitCounter/8] & (0x01<<(7-(bitCounter%8)))) {
						TCC1.PER = robotData.lowBitTime;
					}
					else {
						TCC1.PER = robotData.highBitTime;
					}					
					bitCounter++;
				}
				else {
					if(robotData.irBytes[bitCounter/8] & (0x01<<(7-(bitCounter%8)))) {
						TCC1.PER = robotData.highBitTime;
					}
					else {
						TCC1.PER = robotData.lowBitTime;
					}

				}
			}
			// Raw time encoding - data is stored a bit differently in irBytes in raw encoding. Each pair of bytes represents
			// one time measurement - so turn the signal on or off by that amount of time.
			else if(robotData.bitEncoding == 0x04) {
				TCC1.PER = ((robotData.irBytes[bitCounter])<<8) + robotData.irBytes[bitCounter+1];
				bitCounter+=2;
			}
		}
	}
}
