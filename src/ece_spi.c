// SPI support

// Written by Bernie Roehl, February 2017

#include <lpc17xx.h>
#include "ECE_SPI.h"

#define PIN_CS (1 << 19)  // chip select is P0.19
#define RNE 0x04          // bit in the status register to poll to see if byte was sent

void SPI_setup(void) {
	// SPI pin configuration
	LPC_PINCON->PINSEL0 &= 0x3FFFFFFF;	 // P0.15 is...
	LPC_PINCON->PINSEL0 |= 0x80000000;   // ... SCK0
	LPC_PINCON->PINSEL1 &= 0xFFFFFFC3;	 // P0.17 and 18 are
	LPC_PINCON->PINSEL1 |= 0x00000028;   // ... MISO0 and MOSI0
	// SPI configuration
	LPC_SSP0->CR0        = 0x01C7;	     // 8-bit transfer, SPI format, CPOL=1, CPHA=1, SCR=1 
 	LPC_SSP0->CR1        = 0x0002;	     // normal operation, SSP enabled, bus master
 	LPC_SSP0->CPSR       = 32;           // clock divisor (IMU needs SCLK < 1 MHz)
	// chip select
	LPC_PINCON->PINSEL1 &= 0xFFFFFF3F;   // clear bits 7:6 so pin P0.19 is GPIO
	LPC_GPIO0->FIODIR   |= PIN_CS;       // set the P0.19 pin as output
	LPC_GPIO0->FIOSET    = PIN_CS;       // set it high initially (it's active low)
}

void SPI_select() {
	LPC_GPIO0->FIOCLR = PIN_CS;  // low to select
}

void SPI_deselect() {
	LPC_GPIO0->FIOSET = PIN_CS;  // high to deselect
}

uint8_t SPI_xfer(uint8_t byte) {
	LPC_SSP0->DR = byte;
	while (!(LPC_SSP0->SR & RNE));  // Wait for send to finish
	return LPC_SSP0->DR;
}

// higher-level routines for reading and writing the MPU 9250 registers

void SPI_writeRegister(uint8_t address, uint8_t data) {
	SPI_select();
	SPI_xfer(address);
	SPI_xfer(data);
	SPI_deselect();
}

uint8_t SPI_readRegister(uint8_t address) {
	uint8_t value;
	SPI_select();
	SPI_xfer(address | 0x80);
	value = SPI_xfer(0);
	SPI_deselect();
	return value;
}

 void SPI_readBytes(uint8_t address, uint8_t* buffer, int nbytes) {
	int i;
	SPI_select();
	SPI_xfer(address | 0x80);
	for (i = 0; i < nbytes; ++i)
		buffer[i] = SPI_xfer(i);
	SPI_deselect();
}
