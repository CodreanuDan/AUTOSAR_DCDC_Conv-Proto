###############################################################################
# Makefile for Arduino core & libraries
###############################################################################

## General options
MCU = atmega328p
CLOCK = 16000000
COMPILER = "Arduino"
ARDUINO = 181
SHELL = C:\Windows\system32\cmd.exe

## Tools general options
CCFLAGS = -w -ffunction-sections -fdata-sections -DARDUINO_ARCH_AVR -DF_CPU=$(CLOCK) -DARDUINO=$(ARDUINO) -mmcu=$(MCU) -I "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino" -std=gnu++11 -fno-threadsafe-statics -I "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/variants/eightanaloginputs" -Os
ARFLAGS = ru

## Processing Tools
CC=avr-gcc
AR=ar

## Build tree:
all:	arduino/arduino.a

arduino/arduino.a:	arduino/abi.cpp.o arduino/CDC.cpp.o arduino/HardwareSerial.cpp.o arduino/HardwareSerial0.cpp.o arduino/HardwareSerial1.cpp.o arduino/HardwareSerial2.cpp.o arduino/HardwareSerial3.cpp.o arduino/hooks.c.o arduino/IPAddress.cpp.o arduino/main.cpp.o arduino/new.cpp.o arduino/PluggableUSB.cpp.o arduino/Print.cpp.o arduino/Stream.cpp.o arduino/Tone.cpp.o arduino/USBCore.cpp.o arduino/WInterrupts.c.o arduino/wiring.c.o arduino/wiring_analog.c.o arduino/wiring_digital.c.o arduino/wiring_pulse.c.o arduino/wiring_pulse.S.o arduino/wiring_shift.c.o arduino/WMath.cpp.o arduino/WString.cpp.o arduino/Core/core.cpp.o arduino/Core/DateStrings.cpp.o arduino/Core/PrintBuffer.cpp.o arduino/Core/PrintList.cpp.o arduino/Core/StreamList.cpp.o arduino/Core/Time.cpp.o arduino/Core/TimeLib.cpp.o arduino/Cpu/cpu.cpp.o 

arduino/abi.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/abi.cpp
	 $(CC) $(CCFLAGS) -o "arduino/abi.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/abi.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/abi.cpp.o"

arduino/CDC.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/CDC.cpp
	 $(CC) $(CCFLAGS) -o "arduino/CDC.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/CDC.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/CDC.cpp.o"

arduino/HardwareSerial.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial.cpp
	 $(CC) $(CCFLAGS) -o "arduino/HardwareSerial.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/HardwareSerial.cpp.o"

arduino/HardwareSerial0.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial0.cpp
	 $(CC) $(CCFLAGS) -o "arduino/HardwareSerial0.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial0.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/HardwareSerial0.cpp.o"

arduino/HardwareSerial1.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial1.cpp
	 $(CC) $(CCFLAGS) -o "arduino/HardwareSerial1.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial1.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/HardwareSerial1.cpp.o"

arduino/HardwareSerial2.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial2.cpp
	 $(CC) $(CCFLAGS) -o "arduino/HardwareSerial2.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial2.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/HardwareSerial2.cpp.o"

arduino/HardwareSerial3.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial3.cpp
	 $(CC) $(CCFLAGS) -o "arduino/HardwareSerial3.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/HardwareSerial3.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/HardwareSerial3.cpp.o"

arduino/hooks.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/hooks.c
	 $(CC) $(CCFLAGS) -o "arduino/hooks.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/hooks.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/hooks.c.o"

arduino/IPAddress.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/IPAddress.cpp
	 $(CC) $(CCFLAGS) -o "arduino/IPAddress.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/IPAddress.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/IPAddress.cpp.o"

arduino/main.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/main.cpp
	 $(CC) $(CCFLAGS) -o "arduino/main.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/main.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/main.cpp.o"

arduino/new.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/new.cpp
	 $(CC) $(CCFLAGS) -o "arduino/new.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/new.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/new.cpp.o"

arduino/PluggableUSB.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/PluggableUSB.cpp
	 $(CC) $(CCFLAGS) -o "arduino/PluggableUSB.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/PluggableUSB.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/PluggableUSB.cpp.o"

arduino/Print.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/Print.cpp
	 $(CC) $(CCFLAGS) -o "arduino/Print.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/Print.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Print.cpp.o"

arduino/Stream.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/Stream.cpp
	 $(CC) $(CCFLAGS) -o "arduino/Stream.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/Stream.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Stream.cpp.o"

arduino/Tone.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/Tone.cpp
	 $(CC) $(CCFLAGS) -o "arduino/Tone.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/Tone.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Tone.cpp.o"

arduino/USBCore.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/USBCore.cpp
	 $(CC) $(CCFLAGS) -o "arduino/USBCore.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/USBCore.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/USBCore.cpp.o"

arduino/WInterrupts.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/WInterrupts.c
	 $(CC) $(CCFLAGS) -o "arduino/WInterrupts.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/WInterrupts.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/WInterrupts.c.o"

arduino/wiring.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring.c
	 $(CC) $(CCFLAGS) -o "arduino/wiring.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/wiring.c.o"

arduino/wiring_analog.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_analog.c
	 $(CC) $(CCFLAGS) -o "arduino/wiring_analog.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_analog.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/wiring_analog.c.o"

arduino/wiring_digital.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_digital.c
	 $(CC) $(CCFLAGS) -o "arduino/wiring_digital.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_digital.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/wiring_digital.c.o"

arduino/wiring_pulse.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_pulse.c
	 $(CC) $(CCFLAGS) -o "arduino/wiring_pulse.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_pulse.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/wiring_pulse.c.o"

arduino/wiring_pulse.S.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_pulse.S
	 $(CC) $(CCFLAGS) -o "arduino/wiring_pulse.S.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_pulse.S"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/wiring_pulse.S.o"

arduino/wiring_shift.c.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_shift.c
	 $(CC) $(CCFLAGS) -o "arduino/wiring_shift.c.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/wiring_shift.c"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/wiring_shift.c.o"

arduino/WMath.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/WMath.cpp
	 $(CC) $(CCFLAGS) -o "arduino/WMath.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/WMath.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/WMath.cpp.o"

arduino/WString.cpp.o:	C:/Program\ Files\ (x86)/Labcenter\ Electronics/Proteus\ 8\ Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/WString.cpp
	 $(CC) $(CCFLAGS) -o "arduino/WString.cpp.o" -c "C:/Program Files (x86)/Labcenter Electronics/Proteus 8 Professional/Tools/Arduino/hardware/arduino/avr/cores/arduino/WString.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/WString.cpp.o"

arduino/Core/core.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/core.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/core.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/core.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/core.cpp.o"

arduino/Core/DateStrings.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/DateStrings.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/DateStrings.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/DateStrings.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/DateStrings.cpp.o"

arduino/Core/PrintBuffer.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/PrintBuffer.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/PrintBuffer.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/PrintBuffer.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/PrintBuffer.cpp.o"

arduino/Core/PrintList.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/PrintList.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/PrintList.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/PrintList.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/PrintList.cpp.o"

arduino/Core/StreamList.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/StreamList.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/StreamList.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/StreamList.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/StreamList.cpp.o"

arduino/Core/Time.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/Time.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/Time.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/Time.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/Time.cpp.o"

arduino/Core/TimeLib.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Core/TimeLib.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core" -o "arduino/Core/TimeLib.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Core/TimeLib.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Core/TimeLib.cpp.o"

arduino/Cpu/cpu.cpp.o:	D:/PROTEUS_NOU/Proteus\ 8\ Professional/DATA/VSM\ Studio/drivers/Arduino/Cpu/cpu.cpp
	 $(CC) $(CCFLAGS) @arduino/libs.inc -I "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Cpu" -o "arduino/Cpu/cpu.cpp.o" -c "D:/PROTEUS_NOU/Proteus 8 Professional/DATA/VSM Studio/drivers/Arduino/Cpu/cpu.cpp"
	 $(AR) $(ARFLAGS) arduino/arduino.a "arduino/Cpu/cpu.cpp.o"

clean:	tidy
	 rm -f arduino/arduino.a

tidy:
	 rm -f "arduino/abi.cpp.o"
	 rm -f "arduino/CDC.cpp.o"
	 rm -f "arduino/HardwareSerial.cpp.o"
	 rm -f "arduino/HardwareSerial0.cpp.o"
	 rm -f "arduino/HardwareSerial1.cpp.o"
	 rm -f "arduino/HardwareSerial2.cpp.o"
	 rm -f "arduino/HardwareSerial3.cpp.o"
	 rm -f "arduino/hooks.c.o"
	 rm -f "arduino/IPAddress.cpp.o"
	 rm -f "arduino/main.cpp.o"
	 rm -f "arduino/new.cpp.o"
	 rm -f "arduino/PluggableUSB.cpp.o"
	 rm -f "arduino/Print.cpp.o"
	 rm -f "arduino/Stream.cpp.o"
	 rm -f "arduino/Tone.cpp.o"
	 rm -f "arduino/USBCore.cpp.o"
	 rm -f "arduino/WInterrupts.c.o"
	 rm -f "arduino/wiring.c.o"
	 rm -f "arduino/wiring_analog.c.o"
	 rm -f "arduino/wiring_digital.c.o"
	 rm -f "arduino/wiring_pulse.c.o"
	 rm -f "arduino/wiring_pulse.S.o"
	 rm -f "arduino/wiring_shift.c.o"
	 rm -f "arduino/WMath.cpp.o"
	 rm -f "arduino/WString.cpp.o"
	 rm -f "arduino/Core/core.cpp.o"
	 rm -f "arduino/Core/DateStrings.cpp.o"
	 rm -f "arduino/Core/PrintBuffer.cpp.o"
	 rm -f "arduino/Core/PrintList.cpp.o"
	 rm -f "arduino/Core/StreamList.cpp.o"
	 rm -f "arduino/Core/Time.cpp.o"
	 rm -f "arduino/Core/TimeLib.cpp.o"
	 rm -f "arduino/Cpu/cpu.cpp.o"

