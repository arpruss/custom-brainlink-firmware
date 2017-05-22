/** Includes everything needed for Brainlink's mainFirmware to run */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "clock.h"
#include "led.h"
#include "uart.h"
#include "adc.h"
#include "ir.h"
#include "bluetooth.h"
#include "twi_master_driver.h"
#include "accelerometer.h"
#include "dac.h"
#include "buzzer.h"
#include "ext_pwm.h"
#include "ext_dio.h"
#include "ir_reader.h"
#include "eeprom_driver.h"
#include "dump_sensors.h"

#include "clock.c"
#include "led.c"
#include "uart.c"
#include "adc.c"
#include "ir.c"
#include "bluetooth.c"
#include "twi_master_driver.c"
#include "accelerometer.c"
#include "dac.c"
#include "buzzer.c"
#include "ext_pwm.c"
#include "ext_dio.c"
#include "ir_reader.c"
#include "eeprom_driver.c"
#include "dump_sensors.c"

#ifdef ROOMBA_57K
# define ROOMBA_UART_SETTINGS    135, -2   // 57600 for Roomba Create and older Roombas
#else
# define ROOMBA_UART_SETTINGS    131, -3   // 115200 for Roomba 500
#endif
