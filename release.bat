C:\WinAVR-20100110\utils\bin\make.exe -f Makefile-57k-roomba
if errorlevel 1 goto end
copy mainFirmware.hex 57k-roomba
copy mainFirmware.hex AndroidUploader\assets\57k-roomba.hex
touch brainlink.h
C:\WinAVR-20100110\utils\bin\make.exe
copy mainFirmware.hex AndroidUploader\assets\mainFirmware.hex
:end