The Brainlink uses a lightly modified version of Xboot, an excellent bootloader for the Xmega series. 

Our specific modifications are to xboot.h and the makefile: we have selected the USE_ENTER_PIN method for entering the bootloader, and have configured the bootloader to load over our bluetooth serial port and to blink the blue LED on the Brainlink when writing the firmware.

More documentation on Xboot is available at: http://code.google.com/p/avr-xboot/wiki/Documentation

Xboot is distributed under the MIT License: http://www.opensource.org/licenses/mit-license.php

Note that unlike the regular firmware, the Brainlink's bootloader can not be modified without opening the plastic shell and using an AVR-ISP or similar hardware tool to reprogram it.