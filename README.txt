The files contained in this folder contain the full source of the firmware running on Brainlink's ATXmega16a4 controller. You are free to modify the Brainlink's firmware as desired.

Each source file is commented, but you may wish to begin exploring the code with mainFirmware.c, which is the location of the main function and the implementation of the communications protocol. All other files contain helper functions addressing specific modules of the Brainlink.

In order to customize the firmware, you will need to download a development environment for Atmel's line of Xmega controllers. Instructions for doing so are available in the customizing firmware tutorial at:
http://www.brainlinksystem.com/tutorials

The xboot folder contains the bootloader used to allow re-programming of the Brainlink's main firmware. You may modify the bootloader as well, but you will need to open Brainlink's case and attach hardware programmer (like the AVR-ISP mkII) to the ISP port to do so.

Changes by Alexander Pruss:
- add data streaming
- fix bug reading ADC AUX5
- load calibration data for ADC before use
- add wave generator APIs:

  a digitalports(2 bytes) analogports(1 byte)
    Continuously (every 5 ms) stream data from the digital and analog ports. digitalports and analogports are binary bitmasks indicating which ports
    to stream: e.g., digitalports=0x0011 says to stream ports 0 and 8. The ports are streamed in packets that start with ':', then are followed
    by ASCII '0' or '1' for each of the digital ports, and then are followed by ASCII hex 8-bit data of the analog port data. The port data is
    given in order of pin number. Send '*' to terminate the stream.
    
  u pin(1 byte) pull(1 byte)
    Specified pull-up/pull-down/float for the specified digital pin. Default is float (and this may be needed for some other functions).
    The pin is specified in ASCII ('0' through '9'); the pull is 'u' (up), 'd' (down) or 'f' (float).

  w channel(1 byte) type(1 byte) duty(1 byte) amplitude(1 byte) frequency(3 bytes)
    Initate wave generation.
  Where:
    channel: '0' or '1' (goes out via the two analog outputs)
    type: 's' (sine), 'q' (square) or 't' (traingle)
    duty: duty cycle between 0x00 and 0x3F, for square wave only
    amplitude: unsigned value between 0 and 0xFF (0 to 3.3V)
    frequency: in Hz, with maximum practicable frequency 250000, and lower precision for higher frequencies

  @ channel(1 byte)
    Terminate wave generation
  Where:
     channel: '0' or '1'.

  W channel(1 byte) frequency(3 bytes) numpoints(1 byte) data(numpoints bytes)
    Initiate arbitrary wave generation.
  Where:
    channel: '0' or '1'
    frequency: in Hz, with maximum practicable frequency 250000, and lower precision for higher frequencies;
        longer sequences require lower frequencies
    numpoints: between 0x00 and 0x3F
    data: unsigned values between 0x00 and 0xFF (0 to 3.3V)

  Note: Waveform on channel 0 incompatible with buzzer.
        Waveform on channel 1 incompatible with IR transmission.
        
  # state(1 byte)
    Set whether timeout is active.
  Where:
    state: '0' or '1'

- fix buffer overflow bug in 'r' command
- reduced stack usage
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
- add human-usable aux port baud rate command 'u', followed by two ASCII digits indicating the baud rate.  Valid values are:
  u12: 1200 baud
  u48: 4800 baud
  u96: 9600 baud
  u19: 19200 baud
  u38: 38400 baud
  u57: 57600 baud
  u11: 112500 baud
- space optimizations
- added J1 command to switch the aux uart to irDA mode (with irDA 3/16 duty cycle pulse shaping);  return to default uart settings with J0
