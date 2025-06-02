/*
 * 01_Init_SD.c
 *
 * Using FATFS library to making connect with SD Card.
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "diskio.h"
#include "ff.h"

#define BAUD 9600
#define ubrr (F_CPU / (BAUD * 16UL))

FATFS fs;
FIL file;
DIR dir;
FILINFO fno;

void uart_init(void) {
	UBRR0H = (unsigned char) (ubrr - 1)>>8;
	UBRR0L = (unsigned char) (ubrr - 1);
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_putstring(char* StringPtr) {
	while (*StringPtr != '\0') {
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = *StringPtr++;
	}
}

int main(void) {
	
	uart_init();
	USART_putstring("1. SPI & SD card initialize.\n\r");
	if (disk_initialize(0) != 0) {
		USART_putstring("  - SD card initialize failed.\n\n\r");
		return -1;
	}else {
		USART_putstring("  - SD card initialize success.\n\n\r");
	}

    while (1);
}