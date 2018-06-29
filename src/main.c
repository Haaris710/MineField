#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rtl.h>

// To improve readability
#define BIT2 (0x1 << 2)
#define BIT3 (0x1 << 3)
#define BIT4 (0x1 << 4)
#define BIT5 (0x1 << 5)
#define BIT6 (0x1 << 6)
#define BIT28 (0x1 << 28)
#define BIT29 (0x1 << 29)
#define BIT31 (0x1 << 31)

#define TIMEOUT_INDEFINITE (0xffff)
#define LED_ALL_G1 = (BIT28 | BIT29 | BIT31)
#define LED_ALL_G2 = (BIT2 | BIT3 | BIT4 | BIT5 | BIT6 )



// Map defined as an array of bit maps (global constant)

void ledInit() {
	//set LEDs to output
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	//clear all LEDs
	LPC_GPIO1->FIOCLR |= ALL_LED_G1;
	LPC_GPIO2->FIOCLR |= ALL_LED_G2;
}

void lcdInit() {
	GLCD_Init();
	GLCD_Clear(Blue);
	GLCD_SetBackColor(Blue);
	GLCD_SetTextColor(White);
}

void initialization)() {
	SystemInit();
	ledInit();
	lcdInit();
}

void ledDisplay(uint8_t num) {
	//VARIABLES
	uint32_t buffer = 0;
	
	//Check for invalid input
	if(num > 255 || num < 0)
		return;
	
	//Clear all LEDs
	LPC_GPIO1->FIOCLR |= ALL_LED_G1;
	LPC_GPIO2->FIOCLR |= ALL_LED_G2;
	
	//Build buffer for GPIO1
	buffer |= ((num & BIT7) >> 7) << 28;
	buffer |= ((num & BIT6) >> 6) << 29;
	buffer |= ((num & BIT5) >> 5) << 31;
	
	//Write to register for GPIO1
	LPC_GPIO1->FIOSET |= buffer;
	buffer = 0;
	
	//Build buffer for GPIO2
	buffer |= ((num & BIT4) >> 4) << 2;
	buffer |= (num & BIT3); //(Right shift three, left shift three)
	buffer |= ((num & BIT2) >> 2) << 4;
	buffer |= ((num & BIT1) >> 1) << 5;
	buffer |= (num & BIT0) << 6; 
	
	//Write to register for GPIO1
	LPC_GPIO2->FIOSET |= buffer;
	buffer = 0;
}

int main(void) {
	initialization();
}