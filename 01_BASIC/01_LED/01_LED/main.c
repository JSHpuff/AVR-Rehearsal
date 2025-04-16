/*	01_LED
	Learning Objects:
	1. Port I/O
	2. DDR (Data Direction)
	3. Xor/Toggle
*/

/* Define the CPU's Frequency*/
#define Mhz 1000000UL
#define F_CPU 16*Mhz

/* Including the using libraries */
#include <avr/io.h>
#include <util/delay.h>

/* Main Function */
int main(void){
	/* Set PORT D Data Direction as Output */
	DDRD = 0xff;
	while(1){
		/*	Simple LED: Output to PORT D D2 */
		PORTD |= (1<<PORTD2);	// D2 Set 1
		PORTD ^= (1<<PORTD3);	// Xor/Toggle LED
		_delay_ms(500);			// 500ms
		PORTD &= ~(1<<PORTD2);	// D2 Set 0
		_delay_ms(500);
	}
}