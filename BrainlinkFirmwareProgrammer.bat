@ECHO OFF
if not %1!==! set comPort=%1
if %1!==! set /p comPort=Please enter the com port of the attached Brainlink:
avrdude -p atxmega16a4 -P %comPort% -c avr109  -b 115200 -U flash:w:mainFirmware.hex
PAUSE