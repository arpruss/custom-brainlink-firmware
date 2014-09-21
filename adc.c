#include "adc_driver.c"

uint8_t adcResolution = ADC_RESOLUTION_8BIT_gc;
uint8_t adcConvMode = ADC_ConvMode_Unsigned;
uint8_t adcGain = ADC_DRIVER_CH_GAIN_NONE;
uint8_t adcInputMode = ADC_CH_INPUTMODE_SINGLEENDED_gc;

void init_adc() {
        ADCA.CALL = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL0_offset );
        ADCA.CALH = SP_ReadCalibrationByte( PROD_SIGNATURES_START + ADCACAL1_offset );

	ADC_ConvMode_and_Resolution_Config(&ADCA, adcConvMode, adcResolution);

	/* Set sample rate. */
	ADC_Prescaler_Config(&ADCA, ADC_PRESCALER_DIV512_gc);

	/* Set reference voltage on ADC A to be AREFB (VCC).*/
	ADC_Reference_Config(&ADCA, ADC_REFSEL_AREFB_gc);

	/* Setup channel 1 and 2 with different inputs. */

	// Channel 1 for the temperature sensor
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH1,
	                                 ADC_CH_INPUTMODE_INTERNAL_gc,
	                                 ADC_DRIVER_CH_GAIN_NONE);

	// Channel 2 for all other ADC
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH2,
	                                 adcInputMode,
                                         adcGain);

		/* Set input to the channels in ADC A */
	ADC_Ch_InputMux_Config(&ADCA.CH1, ADC_CH_MUXINT_TEMP_gc, 0);
	ADC_Ch_InputMux_Config(&ADCA.CH2, BATT_VOLT, 0);

	/*  Setup internal temperature reference */
	ADC_TempReference_Enable(&ADCA);

	// Enable the ADC
	ADC_Enable(&ADCA);
	_delay_ms(1);
}

uint16_t read_differential(char port1, char port2) {
    ADC_CH_MUXPOS_t sensor;
    ADC_CH_MUXNEG_t compareSensor;

    switch(port1) {
        case '0':
            sensor = ADC_CH_MUXPOS_PIN0_gc;
            break;
        case '1':
            sensor = ADC_CH_MUXPOS_PIN1_gc;
            break;
        case '2':
            sensor = ADC_CH_MUXPOS_PIN2_gc;
            break;
        case '3':
            sensor = ADC_CH_MUXPOS_PIN3_gc;
            break;
        case '4':
            sensor = ADC_CH_MUXPOS_PIN4_gc;
            break;
        case '5':
            sensor = ADC_CH_MUXPOS_PIN5_gc;
            break;
        default:
            return 0xFFFF;
    }

    if ((port2 < '4' && adcInputMode == ADC_CH_INPUTMODE_DIFFWGAIN_gc) ||
         (port2 >= '4' && adcInputMode == ADC_CH_INPUTMODE_DIFF_gc)) {
         return 0xFFFF;
    }

    switch(port2) {
        case '0':
            compareSensor = ADC_CH_MUXNEG_PIN0_gc;
            break;
        case '1':
            compareSensor = ADC_CH_MUXNEG_PIN1_gc;
            break;
        case '2':
            compareSensor = ADC_CH_MUXNEG_PIN2_gc;
            break;
        case '3':
            compareSensor = ADC_CH_MUXNEG_PIN3_gc;
            break;
        case '4':
            compareSensor = ADC_CH_MUXNEG_PIN4_gc;
            break;
        case '5':
            compareSensor = ADC_CH_MUXNEG_PIN5_gc;
            break;
        default:
            return 0;
    }

    return read_analog(sensor, compareSensor);
}

// Used by mainFirmware to read the analog sensors - light, battery voltage, and auxiliary
int read_analog(ADC_CH_MUXPOS_t sensor, ADC_CH_MUXNEG_t compareSensor) {
	// Set MUX to the right input

        ADC_Ch_InputMux_Config(&ADCA.CH2, sensor, compareSensor);

	//Start conversion
	ADCA.CH2.CTRL |= START_CH;
	do{
	//i++;
			/* If the conversion on the ADCA channel 2 never is
			 * complete this will be a deadlock. */
	}while(!(ADCA.CH2.INTFLAGS & 0x01)&& !(ADCA.INTFLAGS & 0x04));

	// Reset interrupt flag for next conversion
	ADCA.CH2.INTFLAGS = ADC_CH_CHIF_bm;
	//ADCA.INTFLAGS |= 0x04;

	// Return 8-bit value of sensor
	if (ADC_RESOLUTION_8BIT_gc == adcResolution) {
	    return ADCA.CH2.RESL;
	}
        else {
            uint8_t h = ADCA.CH2.RESH;
            uint8_t l = ADCA.CH2.RESL;
            return ((uint16_t)h << 8) | l;
        }
}

// Not currently documented as internal temperature is not properly calibrated.
int read_internal_temperature() {
	// Start conversion
	ADCA.CH1.CTRL |= START_CH;
	do{
			/* If the conversion on the ADCA channel 1 never is
			 * complete this will be a deadlock. */
	}while(!(ADCA.CH1.INTFLAGS & 0x01));

	// Reset interrupt flag for next conversion
	ADCA.CH1.INTFLAGS = ADC_CH_CHIF_bm;

	// Return the sensor's value
	return ADCA.CH1.RESL;
}

