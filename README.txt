The files contained in this folder contain the full source of the firmware running on Brainlink's ATXmega16a4 controller. You are free to modify the Brainlink's firmware as desired.

Each source file is commented, but you may wish to begin exploring the code with mainFirmware.c, which is the location of the main function and the implementation of the communications protocol. All other files contain helper functions addressing specific modules of the Brainlink.

In order to customize the firmware, you will need to download a development environment for Atmel's line of Xmega controllers. Instructions for doing so are available in the customizing firmware tutorial at:
http://www.brainlinksystem.com/tutorials

The xboot folder contains the bootloader used to allow re-programming of the Brainlink's main firmware. You may modify the bootloader as well, but you will need to open Brainlink's case and attach hardware programmer (like the AVR-ISP mkII) to the ISP port to do so.