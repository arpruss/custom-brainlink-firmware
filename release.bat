touch brainlink.h mainFirmware.c
C:\WinAVR-20100110\utils\bin\make.exe ROOMBADEFS=-DROOMBA_57K
if errorlevel 1 goto end
copy mainFirmware.hex 57k-roomba
copy mainFirmware.hex AndroidUploader\assets\57k-roomba.hex
touch brainlink.h mainFirmware.c
C:\WinAVR-20100110\utils\bin\make.exe
copy mainFirmware.hex AndroidUploader\assets\mainFirmware.hex
:end
