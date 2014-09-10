/**
  Main program running on the Brainlink. Initializes subsystems, and implements the communications protocol. 
  
  Author: Tom Lauwers tlauwers@birdbraintechnologies.com
  */

// Includes all header files for libraries/drivers
#include "brainlink.h"

#define TEMP_BUFFER_SIZE 31

int main(void)
{
	uint8_t sensor[6]; // Stores analog readings for transmission
	int i; // multipurpose counter
	char choice = 0; // Holds the top-level menu character
	int red = 0;     // For the red LED intensity
	int green = 0;   // For the green LED intensity
	int blue = 0;    // For the blue LED intensity
	int exit = 0;    // Flag that gets set if we need to go back to idle mode.
	unsigned int temph=0;  // Temporary variable (typically stores high byte of a 16-bit int )
	unsigned int templ=0;  // Temporary variable (typically stores low byte of a 16-bit int)
	uint8_t location = 0;  // Holds the EEPROM location of a stored IR signal
	unsigned int frequency; // For PWM control - PWM frequency, also used to set buzzer frequency
	unsigned int duty;      // For PWM control - PWM duty cycle

	int baud;   // Baud rate selection for auxiliary UART
	char scale;  // For auxiliary UART

	uint8_t temp_array[TEMP_BUFFER_SIZE]; // Array to hold received bytes while we count how many are in the buffer
	uint8_t count_buff = 0; // Counter for receive buffer

	
	long int time_out=0; // Counter which counts to a preset level corresponding to roughly 1 minute
	
	// Initialize system
	init_clock();

	init_led();

	init_adc();

	init_ir();

	init_BT();

	init_dac();

	init_buzzer();

	initAccel();

	init_aux_uart(131, -3); // Set the auxiliary uart to 9600 8n1

	EEPROM_DisableMapping();

	// Enable global interrupts
	sei();

	// Do the following indefinitely
	while(1) {
	    // Turn off LED
		set_led(0,0,0);

		// Reset flags (in case this isn't the first time through the loop)
		exit = 0;
		choice = 0;
		time_out = 0;
		stop_ir_timer();

		// Sing a BL song in idle mode so you can be found. Stop as soon as you get a *
		while(choice != 42) {
			uart_putchar(&BT_USART, 'B');
			uart_putchar(&BT_USART, 'L');
			if (USART_RXBufferData_Available(&BT_data)) {
				choice = USART_RXBuffer_GetByte(&BT_data);
				if (choice == 128) {
                                    // Something is trying to connect directly to an iRobot
                                    set_aux_baud_rate( ROOMBA_UART_SETTINGS ); // depends on model
                                    uart_putchar(&AUX_USART, 128); // pass through to iRobot
                                    serial_bridge(); // currently never returns
                                }
                                else if (choice == 0) {
                                    // Something may be trying to connect directly to a MindFlex headset
                                    set_aux_baud_rate( 135, -2); // 57600
                                    uart_putchar(&AUX_USART, 0); // pass through to headset
                                    serial_bridge(); // currently never returns;
                                }
                                else {
				    uart_putchar(&BT_USART, choice);
                                }
			}
			if (choice != 42)
                            _delay_ms(500);
		}

		// Active part of the program - listens for commands and responds as necessary
		while(exit == 0) {
			// Checks if we haven't heard anything for a long time, in which case we exit loop and go back to idle mode
			time_out++;
			// Corresponds to roughly 60 seconds
			if(time_out > 33840000) {
				exit = 1;
			}

			// Check for a command character
			if (USART_RXBufferData_Available(&BT_data)) {
				choice = USART_RXBuffer_GetByte(&BT_data);
			}
			else {
				choice = 0;
			}
			// If it exists, act on command
			if(choice != 0) {
				// Reset the time out
				time_out = 0;
				// Return the command so the host knows we got it
				uart_putchar(&BT_USART, choice);

				// Giant switch statement to decide what to do with the command
				switch(choice) {
					// Return the currect accelerometer data - X, Y, Z, and status (contains tapped and shaken bits)
					case 'A':
						updateAccel();
						uart_putchar(&BT_USART, _acc.x);
						uart_putchar(&BT_USART, _acc.y);
						uart_putchar(&BT_USART, _acc.z);
						uart_putchar(&BT_USART, _acc.status);
						break;
					// Set the buzzer
					case 'B':
						temph = uart_getchar_timeout(&BT_USART);
						// If temph is 256, it means we didn't get a follow up character and timed out, so respond with ERR
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						frequency = ((temph)<<8) + templ;
						set_buzzer(frequency);
						break;
					// Turn off the buzzer
					case 'b':
						turn_off_buzzer();
						break;
					// Returns the value of the light sensor
					case 'L':
						sensor[0] = read_analog(LIGHT);
						uart_putchar(&BT_USART, sensor[0]);
						break;
					// Returns the Xmegas internal temperature read - this is undocumented because the value returned is very erratic
					case 'T':
						sensor[0] = read_internal_temperature();
						uart_putchar(&BT_USART, sensor[0]);
						break;
					// Returns the battery voltage
					case 'V':
						sensor[0] = read_analog(BATT_VOLT);
						uart_putchar(&BT_USART, sensor[0]);
						break;
					// Returns the readings on all six ADC ports
					case 'X':
						sensor[0] = read_analog(AUX0);
						uart_putchar(&BT_USART, sensor[0]);
						sensor[1] = read_analog(AUX1);
						uart_putchar(&BT_USART, sensor[1]);
						sensor[2] = read_analog(AUX2);
						uart_putchar(&BT_USART, sensor[2]);
						sensor[3] = read_analog(AUX3);
						uart_putchar(&BT_USART, sensor[3]);
						sensor[4] = read_analog(AUX4);
						uart_putchar(&BT_USART, sensor[4]);
						sensor[5] = read_analog(AUX4);
						uart_putchar(&BT_USART, sensor[5]);
						break;
					// Sets the full-color LED
					case 'O':
						red = uart_getchar_timeout(&BT_USART);
						if(red == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, red);
						}
						green = uart_getchar_timeout(&BT_USART);
						if(green == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, green);
						}
						blue = uart_getchar_timeout(&BT_USART);
						if(blue == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, blue);
						}
						set_led(red, green, blue);
						break;
				        // Switches serial between irDA and standard serial
				        // Currently, this works over the same port numbers as
				        // standard serial, instead of using the IR receiver
				        // and IR LED.  This may change when I get the IR receiver
				        // and LED working reliably, so don't count on this
				        // functionality.
				        case 'J':
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
                                                disable_aux_uart();
						if (temph == '1') {
                                                    init_aux_uart_ir(829, -2); // 9600 baud
                                                }
                                                else {
                                                    init_aux_uart(131, -3); // 115200 baud
                                                }
                                                break;

					// Sets up the IR transmitter with signal characteristics
					case 'I':
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Set the frequency of the IR carrier
						robotData.frequency = ((temph)<<8) + templ;
						set_ir_carrier(robotData.frequency);
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
						// Set the length of the start up pulses
							robotData.startUpPulseLength = templ;
							uart_putchar(&BT_USART, robotData.startUpPulseLength);
						}
						if(robotData.startUpPulseLength > 16) {
							uart_putchar(&BT_USART, 'R');
							break;
						}

						// Read in the start up pulse timing data
						for(i=0; i < robotData.startUpPulseLength; i++) {
							temph = uart_getchar_timeout(&BT_USART);
							if(temph == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, temph);
							}
							templ = uart_getchar_timeout(&BT_USART);
							if(templ == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, templ);
							}
							robotData.startUpPulse[i] = ((temph)<<8) + templ;
						}
						if(temph == 256 || templ == 256) {
							break;
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
						// Set the bit encoding to one of four pre-determined settings (see protocol instructions for more information)
							robotData.bitEncoding = templ;
							uart_putchar(&BT_USART, robotData.bitEncoding);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Set the number of bits (and bytes) contained in an IR command
						robotData.numBits = templ;
						robotData.numBytes = (robotData.numBits-1)/8 + 1;
						uart_putchar(&BT_USART, robotData.numBits);
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Set timing data for a high bit
						robotData.highBitTime = ((temph)<<8) + templ;
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Set timing data for a low bit
						robotData.lowBitTime = ((temph)<<8) + templ;
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Set timing data for on or off
						robotData.offTime = ((temph)<<8) + templ;
						break;
					// Transmit an IR signal according to the previously determined configuration
					case 'i':
						// Get the signal data as one or more bytes
						for(i = 0; i < robotData.numBytes; i++) {
							templ = uart_getchar_timeout(&BT_USART);
							if(templ == 256) {
                                                                err();
								break;
							}
							else {
								robotData.irBytes[i] = templ;
								uart_putchar(&BT_USART, robotData.irBytes[i]);
							}
						}
						if(templ == 256) {
							break;
						}
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Determine if the signal is repeated or not, and if so, with what frequency
						robotData.repeatTime = ((temph)<<8) + templ;
						if(robotData.repeatTime != 0) {
							robotData.repeatFlag = 1;
						}
						else {
							robotData.repeatFlag = 0;
						}
						// Startup timer interrupts
						start_ir_timer();
						break;
					// Turn off any repeating IR signal
					case '!':
						robotData.repeatFlag = 0;
						stop_ir_timer();
						break;
					// Capture a signal from the IR receiver
					case 'R':
						init_ir_read();
						while(ir_read_flag!=0);
						break;
					// Store the captured signal in an EEPROM location
					case 'S':
						location = uart_getchar_timeout(&BT_USART)-48; // Subtracing 48 converts from ASCII to numeric numbers
						if((location >= 0) && (location < 5) && (signal_count > 4)) {
							uart_putchar(&BT_USART, location+48);
							write_data_to_eeprom(location);
						}
						else {
                                                        err();
						}
						break;
					// Receive a raw IR signal over bluetooth and transmit it with the IR LED
					case 's':
						if(read_data_from_serial()) {
							temph = uart_getchar_timeout(&BT_USART);
							if(temph == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, temph);
							}
							templ = uart_getchar_timeout(&BT_USART);
							if(templ == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, templ);
							}
							// Set if the signal should repeat and if so, with what frequency
							robotData.repeatTime = ((temph)<<8) + templ;
							if(robotData.repeatTime != 0) {
								robotData.repeatFlag = 1;
							}
							else {
								robotData.repeatFlag = 0;
							}
							// Set frequency to 38KHz, since raw signals must have come from the receiver at some point
							robotData.frequency = 0x0349;
							robotData.startUpPulseLength = 0;
							robotData.bitEncoding = 0x04;
							start_ir_timer();
						}
						else {
                                                                err();
								break;
						}
						break;
					// Get a stored signal from an EEPROM location and transmit it over the IR LED (and repeat as desired)
					case 'G':
						location = uart_getchar_timeout(&BT_USART)-48;
						if(location >= 0 && location < 5) {
							uart_putchar(&BT_USART, location+48);
							temph = uart_getchar_timeout(&BT_USART);
							if(temph == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, temph);
							}
							templ = uart_getchar_timeout(&BT_USART);
							if(templ == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, templ);
							}
							robotData.repeatTime = ((temph)<<8) + templ;
							if(robotData.repeatTime != 0) {
								robotData.repeatFlag = 1;
							}
							else {
								robotData.repeatFlag = 0;
							}
							read_data_from_eeprom(location);
							robotData.frequency = 0x0349;
							robotData.startUpPulseLength = 0;
							robotData.bitEncoding = 0x04;
							start_ir_timer();
						}
						else {
                                                        err();
						}
						break;
					// Get a stored signal from EEPROM and print it over bluetooth to the host
					case 'g':
						location = uart_getchar_timeout(&BT_USART)-48;
						if(location >= 0 && location < 5) {
							uart_putchar(&BT_USART, location+48);
							print_data_from_eeprom(location);
						}
						else {
                                                        err();
						}
						break;
						// Output on digital I/O
					case '>':
						// Set port
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						// Get value
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						set_output(temph, (templ-48));
						break;
						// Input on digital I/O
					case '<':
						// Get port
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						// Get value (1 or 0)
						templ = read_input(temph)+48;
						uart_putchar(&BT_USART, templ);
						break;
					// Configure PWM frequency
					case 'P':
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						// Stores the PWM frequency for use by set_pwm()
						pwm_frequency = ((temph)<<8) + templ;
						break;
					// Set PWM duty cycle for a specific port
					case 'p':
						set_pwm();
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
							if(temph == '0') {
								temph = uart_getchar_timeout(&BT_USART);
								if(temph == 256) {
                                                                        err();
									break;
								}
								else {
									uart_putchar(&BT_USART, temph);
								}
								templ = uart_getchar_timeout(&BT_USART);
								if(templ == 256) {
                                                                        err();
									break;
								}
								else {
									uart_putchar(&BT_USART, templ);
								}
								duty = ((temph)<<8) + templ;
								set_pwm0(duty);
							}
							else if(temph == '1') {
								temph = uart_getchar_timeout(&BT_USART);
								if(temph == 256) {
                                                                        err();
									break;
								}
								else {
									uart_putchar(&BT_USART, temph);
								}
								templ = uart_getchar_timeout(&BT_USART);
								if(templ == 256) {
                                                                        err();
									break;
								}
								else {
									uart_putchar(&BT_USART, templ);
								}
								duty = ((temph)<<8) + templ;
								set_pwm1(duty);
							}
						}
						break;
					// Set DAC voltage on one of the two DAC ports
					case 'd':
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
							if(temph == '0') {
								temph = uart_getchar_timeout(&BT_USART);
								if(temph == 256) {
                                                                        err();
									break;
								}
								else {
									uart_putchar(&BT_USART, temph);
									set_dac0(temph);
								}
							}
							else if(temph == '1') {
								temph = uart_getchar_timeout(&BT_USART);
								if(temph == 256) {
                                                                        err();
									break;
								}
								else {
									uart_putchar(&BT_USART, temph);
									set_dac1(temph);
								}
							}
						}
						break;
						// Go back to idle mode so you can be found again. Should be sent at end of host program.
					case 'Q':
						exit = 1;
						break;
						// Human-readable uart speed setting
					case 'u':
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						baud = ((temph)<<8) + templ;
						switch(baud) {
                                                    case ('1'<<8) + '2': // 1200
                                                        baud = 6663;
                                                        scale = -2;
                                                        break;
                                                    case ('4'<<8) + '8': // 4800
                                                        baud = 3325;
                                                        scale = -3;
                                                        break;
                                                    case ('9'<<8) + '6': // 9600
                                                        baud = 829;
                                                        scale = -2;
                                                        break;
                                                    case ('1'<<8) + '9': // 19200
                                                        baud = 825;
                                                        scale = -3;
                                                        break;
                                                    case ('3'<<8) + '8': // 38400
                                                        baud = 204;
                                                        scale = -2;
                                                        break;
                                                    case ('5'<<8) + '7': // 57600
                                                        baud = 135;
                                                        scale = -2;
                                                        break;
                                                    case ('1'<<8) + '1': // 115200
                                                        baud = 131;
                                                        scale = -3;
                                                        break;
                                                    default:
                                                        err();
                                                        goto BAUD_DONE;
                                                }
						set_aux_baud_rate(baud, scale);

                                                BAUD_DONE:
                                                break;

					// Configures the baud rate of the auxiliary UART
					case 'C':
						temph = uart_getchar_timeout(&BT_USART);
						if(temph == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, temph);
						}
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						baud = ((temph)<<8) + templ;
						templ = uart_getchar_timeout(&BT_USART);
						if(templ == 256) {
                                                        err();
							break;
						}
						else {
							uart_putchar(&BT_USART, templ);
						}
						scale = templ;
						uart_putchar(&BT_USART, scale);
						set_aux_baud_rate(baud, scale);
						break;
					// BT-serial high speed bridge mode
					case 'Z':
					        serial_bridge();
					        break;
					case 'r':
						count_buff = 0;
						while(USART_RXBufferData_Available(&AUX_data) && count_buff < TEMP_BUFFER_SIZE) {
							temp_array[count_buff]= USART_RXBuffer_GetByte(&AUX_data);
							count_buff++;
						}
						uart_putchar(&BT_USART, count_buff+1);
						for(int i = 0; i < count_buff; i++) {
							uart_putchar(&BT_USART, temp_array[i]);
						}
						break;
					// Transmit a stream of characters from bluetooth to auxiliary serial
					case 't':
						temph= uart_getchar_timeout(&BT_USART);
						uart_putchar(&BT_USART, temph);
						for(int count = 0; count < temph; count++) {
							templ= uart_getchar_timeout(&BT_USART);

							if(templ == 256) {
                                                                err();
								break;
							}
							else {
								uart_putchar(&BT_USART, templ);
								uart_putchar(&AUX_USART, templ);
							}
						}
						break;
					default:
						break;
				}
			}
		}
		
	}
	return 0;
}
