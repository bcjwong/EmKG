# EmKG
EC535 Embedded Systems (Spring 2019) Boston University

## Implementation of code files
The following code files need to be implemented to run the EmKG project for class EC535
Introduction to Embedded Systems (Spring 2019) at Boston University.
Arduino
- ADC.ino
Gumstix
- mypulse.c
LCD
- main.cpp
- digitalclock.cpp
- digitalclock.h


### Usage
To get the data from the pulse sensor onto the gumstix for analysis, the ADC.ino provides the code to run the arduino software. After connecting the wires and uploading the code to arduino, the pulse sensor will be able to transmit signals onto the gumstix for processing. The arduino will then transmit this processed signal to the gumstix via 8 binary digits.

Compile the mypulse.c code into mypulse.ko by using a makefile. Place the LCD files,
main.cpp, digitalclock.cpp and digitalclock.h into a file and compile with a makefile. After running minicom, upload the mypulse.ko and executable file - bluescreen. Create library links and export variables in gumstix. <insmod mypulse.ko> should begin the transmission, calculating the bpm, and other functionalities, and running the executable <./blue_screen -qws> should display the graph onto the LCD screen.


#### Authors
Benjamin Wong, Apollo Lo, Gennifer Norman
