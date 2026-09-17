###############################################################################
# Makefile for project 
###############################################################################

## General options
PROJECT = 
MCU = Arduino Uno
COMPILER = "Arduino AVR (Proteus)"
TARGET = Debug
SHELL = C:\WINDOWS\system32\cmd.exe

## Tools general options
APPFLAGS=-C pro328 -T -std=gnu++11 -T -fno-threadsafe-statics -N atmega328p -F 16000000 -U "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino"
CCFLAGS=-Wall -fno-exceptions -ffunction-sections -fdata-sections -DF_CPU=16000000 -I .. -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino" -DARDUINO_ARCH_AVR @arduino/libs.inc -std=gnu++11 -fno-threadsafe-statics -O0 -mmcu=atmega328p
LDFLAGS=-Wl,--gc-sections -lm -mmcu=atmega328p

## Processing Tools
FLOWCHART=
APP=mksketch
CC=avr-gcc
ASM=avr-gcc
LD=avr-gcc
#  C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/Debug.elf: TOOL=LD
#  Post: avr-objcopy -O ihex -R .eeprom "./Debug.elf" "./Debug.hex"
#  Post: avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 --no-change-warnings -O ihex "./Debug.elf" "./Debug.eep" || exit 0 
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/main.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/main.cpp: TOOL=APP
#        Post: make -f arduino/Makefile all
#           C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/main.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/AdcSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/AdcSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/ComSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/ComSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/DcmSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/DcmSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/DemSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/DemSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/DioSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/DioSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/EcuM.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/EcuM.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/GptSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/GptSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/IoHwAb.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/IoHwAb.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/NvM.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/NvM.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/PduR.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/PduR.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/PortSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/PortSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/PwmSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/PwmSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/Rte.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/Rte.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/SWC_PidController.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/SWC_PidController.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/SWC_SensorMeasurement.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/SWC_SensorMeasurement.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/UartSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/UartSf.c
#     C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/ARDUINO UNO/Debug/WdgSf.o: TOOL=CC
#        C:/Users/dan/AppData/Local/Temp/VSM Studio/ad316273f29a43f5890ab3e8823e00a4/Arduino Uno/WdgSf.c

# Build tree:
all:	Debug

Debug:	Debug.elf
	avr-size "Debug.ELF"

Debug.elf:	main.o AdcSf.o ComSf.o DcmSf.o DemSf.o DioSf.o EcuM.o GptSf.o IoHwAb.o NvM.o PduR.o PortSf.o PwmSf.o Rte.o SWC_PidController.o SWC_SensorMeasurement.o UartSf.o WdgSf.o 
	$(LD) $(LDFLAGS)  -o "./Debug.elf" "main.o" "AdcSf.o" "ComSf.o" "DcmSf.o" "DemSf.o" "DioSf.o" "EcuM.o" "GptSf.o" "IoHwAb.o" "NvM.o" "PduR.o" "PortSf.o" "PwmSf.o" "Rte.o" "SWC_PidController.o" "SWC_SensorMeasurement.o" "UartSf.o" "WdgSf.o" arduino/arduino.a -lm
	avr-objcopy -O ihex -R .eeprom "./Debug.elf" "./Debug.hex"
	avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 --no-change-warnings -O ihex "./Debug.elf" "./Debug.eep" || exit 0 

main.o:	main.cpp 
	$(CC) $(CCFLAGS)  -o "main.o" -c "main.cpp"

main.cpp:	../main.c 
	$(APP) $(APPFLAGS)  -A "C:\Program Files (x86)\Labcenter Electronics\Proteus 8 Professional\Tools\Arduino" -O main.cpp "../main.c"
	make -f arduino/Makefile all

AdcSf.o:	../AdcSf.c 
	$(CC) $(CCFLAGS)  -o "AdcSf.o" -c "../AdcSf.c"

ComSf.o:	../ComSf.c 
	$(CC) $(CCFLAGS)  -o "ComSf.o" -c "../ComSf.c"

DcmSf.o:	../DcmSf.c 
	$(CC) $(CCFLAGS)  -o "DcmSf.o" -c "../DcmSf.c"

DemSf.o:	../DemSf.c 
	$(CC) $(CCFLAGS)  -o "DemSf.o" -c "../DemSf.c"

DioSf.o:	../DioSf.c 
	$(CC) $(CCFLAGS)  -o "DioSf.o" -c "../DioSf.c"

EcuM.o:	../EcuM.c 
	$(CC) $(CCFLAGS)  -o "EcuM.o" -c "../EcuM.c"

GptSf.o:	../GptSf.c 
	$(CC) $(CCFLAGS)  -o "GptSf.o" -c "../GptSf.c"

IoHwAb.o:	../IoHwAb.c 
	$(CC) $(CCFLAGS)  -o "IoHwAb.o" -c "../IoHwAb.c"

NvM.o:	../NvM.c 
	$(CC) $(CCFLAGS)  -o "NvM.o" -c "../NvM.c"

PduR.o:	../PduR.c 
	$(CC) $(CCFLAGS)  -o "PduR.o" -c "../PduR.c"

PortSf.o:	../PortSf.c 
	$(CC) $(CCFLAGS)  -o "PortSf.o" -c "../PortSf.c"

PwmSf.o:	../PwmSf.c 
	$(CC) $(CCFLAGS)  -o "PwmSf.o" -c "../PwmSf.c"

Rte.o:	../Rte.c 
	$(CC) $(CCFLAGS)  -o "Rte.o" -c "../Rte.c"

SWC_PidController.o:	../SWC_PidController.c 
	$(CC) $(CCFLAGS)  -o "SWC_PidController.o" -c "../SWC_PidController.c"

SWC_SensorMeasurement.o:	../SWC_SensorMeasurement.c 
	$(CC) $(CCFLAGS)  -o "SWC_SensorMeasurement.o" -c "../SWC_SensorMeasurement.c"

UartSf.o:	../UartSf.c 
	$(CC) $(CCFLAGS)  -o "UartSf.o" -c "../UartSf.c"

WdgSf.o:	../WdgSf.c 
	$(CC) $(CCFLAGS)  -o "WdgSf.o" -c "../WdgSf.c"

# bootloader - attach bootloader to the firmware
bootloader:
	avr-objcopy -O ihex -R .eeprom "./Debug.elf" "./Debug.hex"
	copy /b bootloader.hex+"./Debug.hex" "with.bootloader.hex"

# tidy - delete all temporary files which are not involved in the target generation
tidy:
	rm -rf ./main.d
	rm -rf ./AdcSf.d
	rm -rf ./ComSf.d
	rm -rf ./DcmSf.d
	rm -rf ./DemSf.d
	rm -rf ./DioSf.d
	rm -rf ./EcuM.d
	rm -rf ./GptSf.d
	rm -rf ./IoHwAb.d
	rm -rf ./NvM.d
	rm -rf ./PduR.d
	rm -rf ./PortSf.d
	rm -rf ./PwmSf.d
	rm -rf ./Rte.d
	rm -rf ./SWC_PidController.d
	rm -rf ./SWC_SensorMeasurement.d
	rm -rf ./UartSf.d
	rm -rf ./WdgSf.d
	-make -f arduino/Makefile tidy

# cleanup - delete all generated files
clean:	tidy
	rm -rf Debug.elf
	rm -rf main.o
	rm -rf main.cpp
	rm -rf AdcSf.o
	rm -rf ComSf.o
	rm -rf DcmSf.o
	rm -rf DemSf.o
	rm -rf DioSf.o
	rm -rf EcuM.o
	rm -rf GptSf.o
	rm -rf IoHwAb.o
	rm -rf NvM.o
	rm -rf PduR.o
	rm -rf PortSf.o
	rm -rf PwmSf.o
	rm -rf Rte.o
	rm -rf SWC_PidController.o
	rm -rf SWC_SensorMeasurement.o
	rm -rf UartSf.o
	rm -rf WdgSf.o
	-make -f arduino/Makefile clean
