/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"

#define DDR_SPI DDRB
#define DD_MOSI DDB3
#define DD_MISO DDB4
#define DD_SCK  DDB5
#define DD_SS   DDB2

#define CS_LOW()  (PORTB &= ~(1 << PB2))	// Selected
#define CS_HIGH() (PORTB |=  (1 << PB2))	// Unselected

#define CMD_GO_IDLE_STATE 0x00				/* CMD0: response R1 */
#define CMD_SEND_IF_COND 0x08				/* CMD8: response R7 */
#define CMD_SET_BLOCKLEN 0x10				/* CMD16: arg0[31:0]: block length, response R1 */
#define CMD_SD_SEND_OP_COND 0x29			/* CMD41: arg0[31:0]: OCR contents, response R1 */
#define CMD_APP 0x37						/* CMD55: arg0[31:0]: stuff bits, response R1 */
#define CMD_READ_OCR 0x3a					/* CMD58: arg0[31:0]: stuff bits, response R3 */

extern void USART_putstring(char* StringPtr);

void spi_transfer(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	SPSR &= ~(1 << SPIF);
}

/* Receives a raw byte from the memory card */
uint8_t spi_receive_raw() {
	/* send dummy data for receiving some */
	SPDR = 0xff;
	while(!(SPSR & (1 << SPIF)));
	SPSR &= ~(1 << SPIF);
	return SPDR;
}

uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
	uint8_t response;
	
	spi_receive_raw();
	
	/* send command via SPI */
	spi_transfer(0x40 | cmd);
	spi_transfer((arg >> 24) & 0xff);
	spi_transfer((arg >> 16) & 0xff);
	spi_transfer((arg >> 8) & 0xff);
	spi_transfer((arg >> 0) & 0xff);

	switch(cmd)
	{
		case CMD_GO_IDLE_STATE:
		spi_transfer(0x95);
		break;
		case CMD_SEND_IF_COND:
		spi_transfer(0x87);
		break;
		default:
		spi_transfer(0xff);
		break;
	}
	
	/* receive response */
	for(uint8_t i = 0; i < 10; ++i)
	{
		response = spi_receive_raw();
		if(response != 0xff)
		break;
	}

	return response;
}

void SD_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
	// transmit command to sd card
	spi_transfer(cmd|0x40);

	// transmit argument
	spi_transfer((uint8_t)(arg >> 24));
	spi_transfer((uint8_t)(arg >> 16));
	spi_transfer((uint8_t)(arg >> 8));
	spi_transfer((uint8_t)(arg));

	// transmit crc
	spi_transfer(crc|0x01);
}

void spi_init(void) {
	// Set MOSI, SCK, SS as output; MISO as input
	DDR_SPI |= (1 << DD_MOSI) | (1 << DD_SCK) | (1 << DD_SS);
	DDR_SPI &= ~(1 << DD_MISO);

	CS_HIGH();               // Ensure CS is HIGH first

	/* initialize SPI with lowest frequency; max. 400kHz during identification mode of card */
	SPCR =	(0 << SPIE) | /* SPI Interrupt Enable */
	(1 << SPE)  | /* SPI Enable */
	(0 << DORD) | /* Data Order: MSB first */
	(1 << MSTR) | /* Master mode */
	(0 << CPOL) | /* Clock Polarity: SCK low when idle */
	(0 << CPHA) | /* Clock Phase: sample on rising SCK edge */
	(1 << SPR1) | /* Clock Frequency: f_OSC / 128 */
	(1 << SPR0);
	SPSR &= ~(1 << SPI2X); /* No doubled clock frequency */
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv != 0) return STA_NOINIT;

	spi_init();
	CS_HIGH();											// make sure card is deselected
	_delay_ms(1);										// give SD card time to power up
	for(uint8_t i = 0; i < 10; i++) spi_receive_raw();	// wait 80 clock cycles to synchronize

	// Send CMD0 to reset SD card
	CS_LOW();
	uint8_t response;
	for(uint16_t i = 0; ; ++i) {
		response = sd_send_cmd(CMD_GO_IDLE_STATE, 0);
		if(response == 0x01) {
			break;
		}
		if(i == 0x1ff) {
			response = 0;
			USART_putstring("SPI working, but no SD response to CMD0.\n\r");
			return STA_NOINIT;
		}
	}
	CS_HIGH();
	spi_receive_raw();

	/* check for version of SD card specification */
	CS_LOW();
	response = sd_send_cmd(CMD_SEND_IF_COND, 0x1AA);
	uint8_t is_v2 = 0;
	if (response == 0x01) {
		uint8_t r7[4];
		for (uint8_t i = 0; i < 4; i++) r7[i] = spi_receive_raw();
		if ((r7[2] & 0x01) && r7[3] == 0xAA) {
				is_v2 = 1;
			} else {
				USART_putstring("CMD8: invalid voltage/test pattern.\n\r");
				CS_HIGH(); 
				return STA_NOINIT;
			}
	} else {
		USART_putstring("Older SD card.\n\r");
	}
	CS_HIGH();
	spi_receive_raw();

	// ACMD41 loop
	for (uint16_t retry = 0; retry < 1000; retry++) {
		CS_LOW();
		sd_send_cmd(CMD_APP, 0);
		response = sd_send_cmd(CMD_SD_SEND_OP_COND, is_v2 ? 0x40000000 : 0);
		CS_HIGH(); spi_receive_raw();
		if (response == 0x00) break;
		_delay_ms(10);
		if (retry == 999) {
			USART_putstring("ACMD41 failed.\n\r");
			return STA_NOINIT;
		}
	}

	// CMD58: Read OCR
	uint8_t is_sdhc = 0;
	CS_LOW();
	response = sd_send_cmd(CMD_READ_OCR, 0);
	if (response == 0x00) {
		uint8_t ocr[4];
		for (uint8_t i = 0; i < 4; i++) ocr[i] = spi_receive_raw();
		if (ocr[0] & 0x40) is_sdhc = 1;
	}
	CS_HIGH(); 
	spi_receive_raw();

	// CMD16: Set block length
	if (!is_sdhc) {
		CS_LOW();
		response = sd_send_cmd(CMD_SET_BLOCKLEN, 512);
		CS_HIGH(); spi_receive_raw();
		if (response != 0x00) {
			USART_putstring("CMD16 failed.\n\r");
			return STA_NOINIT;
		}
	}

	// Enable high-speed SPI
	SPCR &= ~((1 << SPR1) | (1 << SPR0));
	SPSR |= (1 << SPI2X);

	return 0;  // Success
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	return RES_OK;
}

DWORD get_fattime(void) {
	return ((DWORD)(2024 - 1980) << 25)
	| ((DWORD)1 << 21)
	| ((DWORD)1 << 16)
	| ((DWORD)12 << 11)
	| ((DWORD)0 << 5)
	| ((DWORD)0 >> 1);
}
