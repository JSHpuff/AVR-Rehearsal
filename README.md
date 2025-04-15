# AVR-Rehearsal
For Beginner Training AVR.  
The AVR Atmel chip is using ATmega328P.

## Device
The project is using <h4>Arduino Nano / Arduino UNO R3</h4> to accomplish.

## IDE (Intergrated Development Environment)
Using Microchip Studio on windows system.

## IDE Setup
1. Install [Atmel Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio#Downloads)
2. Install [Arduino (Legacy version)](https://www.arduino.cc/en/software/)
3. Install [TeraTerm](https://github.com/TeraTermProject/teraterm/releases)
4. Setup Microchip Studio  
    a. Open Microchip Studio  
    b. Click the "Tools" at the menu bar  
    c. Choose "External Tools..."  
    d. Name the External Tool in "Title"  
    e. Input in the "Command": 
    avrdude.exe path
    ```diff   
    C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe
    ```    
    f. Input in the "Arguments":
    ``` diff
     -C "C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf"
     -v -patmega328p -carduino-PCOMX -b57600 
     -D -Uflash:w:"$(ProjectDir)Debug\$(TargetName).hex":i
    ```    
    at -PCOMX, input your device's com port number.    
5. After setup, you can flash.


## Outline
1. GPIO (General Purpose Input / Output) through LEDs
2. Timer  
    a. Prescaler Setting  
    b. Normal Mode  
    c. CTC (Clear Timer on Compare Match) Mode