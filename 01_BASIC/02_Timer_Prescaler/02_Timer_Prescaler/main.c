/*	02_Timer_Prescaler
	Learning Objects:
	1. How CPU frequency effect
	2. How is timer's prescaler effect
*/

#define Mhz 1000000UL
#define F_CPU 1*Mhz

#include <avr/io.h>
#include <util/delay.h>

int main(void) {
	/* 
		CLKPR:	Clock Prescaler Register
		CLKPCE: Clock Prescaler Change Enable
		CLKPS2: Clock Prescaler Select Bits 2
	*/
	CLKPR = (1<<CLKPCE);
	CLKPR = (1<<CLKPS2);
	
	DDRD = 0xff;
	while(1){
		PORTD ^= (1<<PORTD2);
		_delay_ms(1000);
	}
}