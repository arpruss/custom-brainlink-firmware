/**
  Main program running on the Brainlink. Initializes subsystems, and implements the communications protocol.

  Author: Tom Lauwers tlauwers@birdbraintechnologies.com
  */

// Includes all header files for libraries/drivers
#include "brainlink.h"

#define MAX_ARGUMENTS 7

uint8_t arguments[MAX_ARGUMENTS];

// Caller has the responsibility of ensuring count <= MAX_ARGUMENTS
uint8_t bt_to_buffer(uint8_t* buffer, uint8_t count) {
    for (uint8_t i = 0 ; i < count ; i++) {
         int x = bt_getchar_timeout_echo();
         if (x == 256) {
             err();
             return 0;
         }
         buffer[i] = (uint8_t)x;
    }

    return count;
}

// Caller has the responsibility of ensuring count <= MAX_ARGUMENTS
uint8_t get_arguments(uint8_t count) {
    return bt_to_buffer(arguments, count);
}

uint32_t get_24bit_argument(uint8_t position) {
    return ((uint32_t)arguments[position] << 16) |
           ((uint32_t)arguments[position+1] << 8) |
           ((uint32_t)arguments[position+2]);
}

#define GET_16BIT_ARGUMENT(position) ( ((uint16_t)arguments[(position)] << 8) | ((uint16_t)arguments[(position)+1]) )

//uint16_t GET_16BIT_ARGUMENT(uint8_t position) {
//    return ((uint16_t)arguments[position] << 8) | (uint16_t)arguments[position+1];
//}

int main(void)
{
        uint8_t sensor[6]; // Stores analog readings for transmission
        int i; // multipurpose counter
        char choice = 0; // Holds the top-level menu character
//      int red = 0;     // For the red LED intensity
//      int green = 0;   // For the green LED intensity
//      int blue = 0;    // For the blue LED intensity
        uint8_t exit = 0;    // Flag that gets set if we need to go back to idle mode.
        unsigned int temph=0;  // Temporary variable (typically stores high byte of a 16-bit int )
        unsigned int templ=0;  // Temporary variable (typically stores low byte of a 16-bit int)
        uint8_t location = 0;  // Holds the EEPROM location of a stored IR signal
//      unsigned int frequency = 0; // For PWM control - PWM frequency, also used to set buzzer frequency
//        int amplitude;
        int channel;
        unsigned int duty;      // For PWM control - PWM duty cycle

        int baud;   // Baud rate selection for auxiliary UART
        char scale;  // For auxiliary UART

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

        init_aux_uart(131, -3); // Set the auxiliary uart to 115200 8n1

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
                        bt_putchar('B');
                        bt_putchar('L');
                        if (USART_RXBufferData_Available(&BT_data)) {
                                choice = USART_RXBuffer_GetByte(&BT_data);
                                if (choice == 128) {
                                    // Something is trying to connect directly to an iRobot
                                    set_aux_baud_rate( ROOMBA_UART_SETTINGS ); // depends on model
                                    aux_putchar( 128); // pass through to iRobot
                                    serial_bridge(); // currently never returns
                                }
                                else if (choice == 0) {
                                    // Something may be trying to connect directly to a MindFlex headset
                                    set_aux_baud_rate( 135, -2); // 57600
                                    aux_putchar( 0); // pass through to headset
                                    serial_bridge(); // currently never returns;
                                }
                                else {
                                    bt_putchar(choice);
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
                                bt_putchar(choice);

                                // Giant switch statement to decide what to do with the command
                                switch(choice) {
                                        // Return the currect accelerometer data - X, Y, Z, and status (contains tapped and shaken bits)
                                        case 'A':
                                                updateAccel();
                                                bt_putchar(_acc.x);
                                                bt_putchar(_acc.y);
                                                bt_putchar(_acc.z);
                                                bt_putchar(_acc.status);
                                                break;
                                        // Set the buzzer
                                        case 'B': // frequency_divider(2)
                                                if (get_arguments(2)) {
                                                    set_buzzer(GET_16BIT_ARGUMENT(0));
                                                }
                                                break;
                                        // Turn off the buzzer
                                        case 'b':
                                                turn_off_buzzer();
                                                break;
                                        // Returns the value of the light sensor
                                        case 'L':
                                                sensor[0] = read_analog(LIGHT, 0);
                                                bt_putchar(sensor[0]);
                                                break;
                                        // Returns the Xmegas internal temperature read - this is undocumented because the value returned is very erratic
                                        case 'T':
                                                sensor[0] = read_internal_temperature();
                                                bt_putchar(sensor[0]);
                                                break;
                                        // Returns the battery voltage
                                        case 'V':
                                                sensor[0] = read_analog(BATT_VOLT, 0);
                                                bt_putchar(sensor[0]);
                                                break;
                                        // Differential ADC mode
                                        case 'D': // active(1) numgainstages(1)
                                                if (get_arguments(2)) {
                                                    if (arguments[0] == '0') {
                                                        adcResolution = ADC_RESOLUTION_8BIT_gc;
                                                        adcConvMode = ADC_ConvMode_Unsigned;
                                                        adcGain = ADC_DRIVER_CH_GAIN_NONE;
                                                        adcInputMode = ADC_CH_INPUTMODE_SINGLEENDED_gc;
                                                        init_adc();
                                                    }
                                                    else if (arguments[0] == '1') {
                                                        adcResolution = ADC_RESOLUTION_12BIT_gc;
                                                        adcConvMode = ADC_ConvMode_Signed;
                                                
                                                        if (arguments[1] >= '1' && arguments[1] <= '6') { // number of gain stages
                                                            adcGain = (arguments[1] - '0') << ADC_CH_GAINFAC_gp;
                                                            adcInputMode = ADC_CH_INPUTMODE_DIFFWGAIN_gc;
                                                            init_adc();
                                                        }
                                                        else if (arguments[1] == '0') {
                                                            adcGain = ADC_DRIVER_CH_GAIN_NONE;
                                                            adcInputMode = ADC_CH_INPUTMODE_DIFF_gc;
                                                            init_adc();
                                                        }
                                                        else {
                                                            err();
                                                        }
                                                    }
                                                    else {
                                                        err();
                                                    }
                                                }
                                                break;
                                        // Returns the readings on all six ADC ports
                                        case 'X':
                                                sensor[0] = read_analog(AUX0, 0);
                                                bt_putchar(sensor[0]);
                                                sensor[1] = read_analog(AUX1, 0);
                                                bt_putchar(sensor[1]);
                                                sensor[2] = read_analog(AUX2, 0);
                                                bt_putchar(sensor[2]);
                                                sensor[3] = read_analog(AUX3, 0);
                                                bt_putchar(sensor[3]);
                                                sensor[4] = read_analog(AUX4, 0);
                                                bt_putchar(sensor[4]);
                                                sensor[5] = read_analog(AUX5, 0);
                                                bt_putchar(sensor[5]);
                                                break;
                                        // Returns differential measurement on pair of ADC ports
                                        // Assumes we are in differential mode with gain
                                        case 'x':
                                                if (get_arguments(2)) {
                                                    i = read_differential(arguments[0], arguments[1]);
                                                    bt_putchar(0xFF&(i >> 8));
                                                    bt_putchar(0xFF&i);
                                                }
                                                break;

                                        case 'e': // this is a bit of testing code, which will eventually be changed
                                                  // do not rely on it
                                                if (get_arguments(2)) {
                                                  char a1 = arguments[0];
                                                  char a2 = arguments[1];
                                                  while(1) {
                                                      _delay_ms(2);
                                                      i = read_differential(a1, a2);
                                                      itoa((i<<4)>>4, arguments, 10);
                                                      for (i=0; arguments[i]; i++)
                                                          bt_putchar(arguments[i]);
                                                      bt_putchar('\r');
                                                      bt_putchar('\n');
                                                  }
                                                }
                                                break;

                                        // Sets the full-color LED
                                        case 'O': // red(1) green(1) blue(1);
                                                if (get_arguments(3)) {
                                                    set_led(arguments[0], arguments[1], arguments[2]);
                                                }
                                                break;
                                        // Switches serial between irDA and standard serial
                                        // Currently, this works over the same port numbers as
                                        // standard serial, instead of using the IR receiver
                                        // and IR LED.  This may change when I get the IR receiver
                                        // and LED working reliably.
                                        case 'J':
                                                temph = bt_getchar_timeout_echo();
                                                if(/*temph == 256 || */ temph < '0' || temph > '1') {
                                                        err();
                                                        break;
                                                }
                                                set_irda_mode(temph - '0');
                                                break;

                                        // Sets up the IR transmitter with signal characteristics
                                        case 'I':
                                                init_ir();
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Set the frequency of the IR carrier
                                                robotData.frequency = ((temph)<<8) + templ;
                                                set_ir_carrier(robotData.frequency);
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                else {
                                                // Set the length of the start up pulses
                                                        robotData.startUpPulseLength = templ;
                                                }
                                                if(robotData.startUpPulseLength > 16) {
                                                        err();
                                                        break;
                                                }

                                                // Read in the start up pulse timing data
                                                for(i=0; i < robotData.startUpPulseLength; i++) {
                                                        temph = bt_getchar_timeout_echo();
                                                        if(temph == 256) {
                                                                err();
                                                                break;
                                                        }
                                                        templ = bt_getchar_timeout_echo();
                                                        if(templ == 256) {
                                                                err();
                                                                break;
                                                        }
                                                        robotData.startUpPulse[i] = ((temph)<<8) + templ;
                                                }
                                                if(temph == 256 || templ == 256) {
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Set the bit encoding to one of four pre-determined settings (see protocol instructions for more information)
                                                robotData.bitEncoding = templ;

                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Set the number of bits (and bytes) contained in an IR command
                                                robotData.numBits = templ;
                                                robotData.numBytes = (robotData.numBits-1)/8 + 1;
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Set timing data for a high bit
                                                robotData.highBitTime = ((temph)<<8) + templ;
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Set timing data for a low bit
                                                robotData.lowBitTime = ((temph)<<8) + templ;
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Set timing data for on or off
                                                robotData.offTime = ((temph)<<8) + templ;
                                                break;
                                        // Transmit an IR signal according to the previously determined configuration
                                        case 'i':
                                                init_ir();
                                                // Get the signal data as one or more bytes
                                                for(i = 0; i < robotData.numBytes; i++) {
                                                        templ = bt_getchar_timeout_echo();
                                                        if(templ == 256) {
                                                                err();
                                                                break;
                                                        }
                                                        robotData.irBytes[i] = templ;
                                                }
                                                if(templ == 256) {
                                                        break;
                                                }
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
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
                                                location = bt_getchar_timeout_echo()-48; // Subtracting 48 converts from ASCII to numeric numbers
                                                if((location >= 0) && (location < 5) && (signal_count > 4)) {
                                                        write_data_to_eeprom(location);
                                                }
                                                else {
                                                        err();
                                                }
                                                break;
                                        // Receive a raw IR signal over bluetooth and transmit it with the IR LED
                                        case 's':
                                                if(read_data_from_serial()) {
                                                        temph = bt_getchar_timeout_echo();
                                                        if(temph == 256) {
                                                                err();
                                                                break;
                                                        }
                                                        templ = bt_getchar_timeout_echo();
                                                        if(templ == 256) {
                                                                err();
                                                                break;
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
                                                location = bt_getchar_timeout_echo()-48;
                                                if(location >= 0 && location < 5) {
                                                        temph = bt_getchar_timeout_echo();
                                                        if(temph == 256) {
                                                                err();
                                                                break;
                                                        }
                                                        templ = bt_getchar_timeout_echo();
                                                        if(templ == 256) {
                                                                err();
                                                                break;
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
                                                location = bt_getchar_timeout_echo()-48;
                                                if(location >= 0 && location < 5) {
                                                        print_data_from_eeprom(location);
                                                }
                                                else {
                                                        err();
                                                }
                                                break;
                                                // Output on digital I/O
                                        case '>':
                                                // Set port
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Get value
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                set_output(temph, (templ-48));
                                                break;
                                                // Input on digital I/O
                                        case '<':
                                                // Get port
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Get value (1 or 0)
                                                templ = read_input(temph)+48;
                                                bt_putchar(templ);
                                                break;
                                        // Configure PWM frequency
                                        case 'P':
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
                                                }
                                                // Stores the PWM frequency for use by set_pwm()
                                                pwm_frequency = ((temph)<<8) + templ;
                                                break;
                                        // Set PWM duty cycle for a specific port
                                        case 'p':
                                                set_pwm();
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == '0') {
                                                        temph = bt_getchar_timeout_echo();
                                                        if(temph == 256) {
                                                            err();
                                                                break;
                                                        }
                                                        templ = bt_getchar_timeout_echo();
                                                        if(templ == 256) {
                                                            err();
                                                                break;
                                                        }
                                                        duty = ((temph)<<8) + templ;
                                                        set_pwm0(duty);
                                                }
                                                else if(temph == '1') {
                                                        temph = bt_getchar_timeout_echo();
                                                        if(temph == 256) {
                                                            err();
                                                                break;
                                                        }
                                                        templ = bt_getchar_timeout_echo();
                                                        if(templ == 256) {
                                                            err();
                                                                break;
                                                        }
                                                        duty = ((temph)<<8) + templ;
                                                        set_pwm1(duty);
                                                }
                                                break;
                                        // Set DAC voltage on one of the two DAC ports
                                        case 'd':
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                        if(temph == '0') {
                                                                temph = bt_getchar_timeout_echo();
                                                                if(temph == 256) {
                                                                        err();
                                                                        break;
                                                                }
                                                                else {
                                                                        set_dac0(temph);
                                                                }
                                                        }
                                                        else if(temph == '1') {
                                                                temph = bt_getchar_timeout_echo();
                                                                if(temph == 256) {
                                                                        err();
                                                                        break;
                                                                }
                                                                else {
                                                                        set_dac1(temph);
                                                                }
                                                        }
                                                break;
                                                // Go back to idle mode so you can be found again. Should be sent at end of host program.
                                        case 'Q':
                                                exit = 1;
                                                break;
                                                // Human-readable uart speed setting
                                        case 'u':
                                                temph = bt_getchar_timeout_echo();
                                                if(temph == 256) {
                                                        err();
                                                        break;
                                                }
                                                templ = bt_getchar_timeout_echo();
                                                if(templ == 256) {
                                                        err();
                                                        break;
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
                                        case 'C': // baud(2) scale(1)
                                          // e.g., 9600 = C\x03\x3D\xFE
                                                if (! get_arguments(3))
                                                    break;
                                                    
                                                bt_putchar(arguments[2]); // duplicate buggy behavior from official firmware; delete if unwanted

                                                set_aux_baud_rate( GET_16BIT_ARGUMENT(0), arguments[2]);
                                                break;
                                        // BT-serial high speed bridge mode
                                        case 'Z':
                                                serial_bridge();
                                                break;
                                        case 'w': // channel(1) type(1) duty(1) amplitude(1) frequency(3)
                                                  // w0s\x20\xFF\x02\x00\x00
                                                if (!get_arguments(7))
                                                     break;
                                                if (arguments[0] < '0' || arguments[0] > '1' || arguments[2] > WAVEFORM_SIZE) {
                                                     err();
                                                     break;
                                                }

                                                play_wave_dac(arguments[0]-'0', (char)arguments[1], arguments[2], arguments[3], get_24bit_argument(4));

                                                break;
                                        case 'W': // channel(1) frequency(3) length(1) data(length)
                                                if (!get_arguments(5))
                                                    break;
                                                if (arguments[0] < '0' || arguments[0] > '1' || arguments[4] > WAVEFORM_SIZE || arguments[4] == 0 ) {
                                                        err();
                                                        break;
                                                }
                                                channel = arguments[0] - '0';
                                                if (bt_to_buffer(waveform[channel], arguments[4])) {
                                                     disable_waveform(channel);
                                                     play_arb_wave(channel, waveform[channel], arguments[4], get_24bit_argument(1));
                                                }
                                                break;

                                        case '@':
                                                channel = bt_getchar_timeout_echo();
                                                if (channel == '0')
                                                    disable_waveform0();
                                                else if (channel == '1')
                                                    disable_waveform1();
                                                else
                                                    err();
                                                break;
                                        case 'r':
                                                i = USART_RXBufferData_AvailableCount(&AUX_data);
                                                bt_putchar((uint8_t)(i+1));
                                                while(i-- > 0) {
                                                        bt_putchar(USART_RXBuffer_GetByte(&AUX_data));
                                                }
                                                break;
                                        // Transmit a stream of characters from bluetooth to auxiliary serial
                                        case 't':
                                                temph= bt_getchar_timeout_echo();
                                                if (temph == 256) {
                                                    err();
                                                    break;
                                                }

                                                for(; temph>0; temph--) {
                                                        templ= bt_getchar_timeout_echo();

                                                        if(templ == 256) {
                                                                err();
                                                                break;
                                                        }
                                                        else {
                                                                aux_putchar( templ);
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
