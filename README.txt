The files contained in this folder contain the full source of the firmware running on Brainlink's ATXmega16a4 controller. You are free to modify the Brainlink's firmware as desired.

Each source file is commented, but you may wish to begin exploring the code with mainFirmware.c, which is the location of the main function and the implementation of the communications protocol. All other files contain helper functions addressing specific modules of the Brainlink.

In order to customize the firmware, you will need to download a development environment for Atmel's line of Xmega controllers. Instructions for doing so are available in the customizing firmware tutorial at:
http://www.brainlinksystem.com/tutorials

The xboot folder contains the bootloader used to allow re-programming of the Brainlink's main firmware. You may modify the bootloader as well, but you will need to open Brainlink's case and attach hardware programmer (like the AVR-ISP mkII) to the ISP port to do so.

Changes by Alexander Pruss:
- remove delay in responding to '*' command
- add serial bridge mode: sending 'Z' initiates a bridge between the Bluetooth and AUX Serial connections using current serial parameters.
  Once bridge mode is initiated, it cannot be terminated except by powercycling the BrainLink.  (I may add a way to exit bridge mode in the future.)
  Bridge mode makes for a much more reliable serial link
- add iRobot mode: sending 0x80 in unconnected mode (e.g., after turning on Brainlink but without sending the '*' command) switches port to 115200 baud
  (for Roomba 500 and newer;  if you want to use the 57600 baud for older Roombas or Create, edit brainlink.h and recompile), passes the 0x80 to the 
  robot and goes into serial bridge mode.  This lets you use the Brainlink as a transparent drop-in replacement for specialized Roomba-Bluetooth bridge
  devices.  Note that Roomba control software that sends AT commands to a bridge and expects a response will not work.  (But as long as the AT commands
  don't contain a '*', the firmware will happily ignore them.)
- add BrainFlex headset mode: sending 0x00 in unconnected mode automatically sets 57600 baud and passes the 0x00 through.
- clear aux port receive buffer whenever the baud rate is changed
- add human-readable aux port baud rate command 'u', followed by two ASCII digits indicating the baud rate.  Currently
  valid commands are:
  u12: 1200 baud
  u96: 9600 baud
  u19: 19200 baud
  u57: 57600 baud
  u11: 112500 baud
- space optimizations
- added J1 command to switch the aux uart to irDA mode (automatically set to 9600 baud, with irDA 3/16
  duty cycle pulse shaping);  return to default uart settings with J0
