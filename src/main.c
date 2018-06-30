#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rtl.h>
#include "GLCD.h"

// To improve readability
#define BIT0 (0x1)
#define BIT1 (0x1 << 1)
#define BIT2 (0x1 << 2)
#define BIT3 (0x1 << 3)
#define BIT4 (0x1 << 4)
#define BIT5 (0x1 << 5)
#define BIT6 (0x1 << 6)
#define BIT7 (0x1 << 7)
#define BIT10 (0x1 << 10)
#define BIT12 (0x1 << 12)
#define BIT18 (0x1 << 18)
#define BIT20 (0x1 << 20)
#define BIT23 (0x1 << 23)
#define BIT24 (0x1 << 24)
#define BIT25 (0x1 << 25)
#define BIT26 (0x1 << 26)
#define BIT27 (0x1 << 27)
#define BIT28 (0x1 << 28)
#define BIT29 (0x1 << 29)
#define BIT30 (0x1 << 30)
#define BIT31 (0x1 << 31)

#define TIMEOUT_INDEFINITE (0xffff)
#define LED_ALL_G1 (BIT28 | BIT29 | BIT31)
#define LED_ALL_G2 (BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define SCALE_FACTOR 16 //map developed in 16x16 blocks

/*
	Map defined as an array of bit maps (global constant)
	
	Map consists of 10, 32-bit numbers representing the vertical
	columns of pixels on a 1:16 scale. Each bit reprents a pixel
	as occupied or unoccupied.
*/

static const uint32_t map[20] = {
	0,0,0,0x19F0,0x19F0,0x8000,0x8000,0x1F00,0x1F46,0x1F46,0x0006,0x0006,
	0xF986,0xF986,0x0186,0x0186,0x3990,0x3818,0,0
};

void ledInit() {
	//set LEDs to output
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	//clear all LEDs
	LPC_GPIO1->FIOCLR |= LED_ALL_G1;
	LPC_GPIO2->FIOCLR |= LED_ALL_G2;
}

/*Prints a 16 pixel square block with parameters 
  as X and Y which represent scaled co-ordinates */
void blockPrint(int x, int y){
	int i = 0;
	int j = 0;
	
	//scaling the co-ordinates
	x = x*SCALE_FACTOR;
	y = y*SCALE_FACTOR;
	
	//printing the block
	for(i=x;i<(x+SCALE_FACTOR-1);i++) {
		for(j=y;j<(y+SCALE_FACTOR-1);j++) {
			GLCD_PutPixel(i, j);
		}
	}
}

/*Clears a 16 pixel square block with parameters 
  as X and Y which represent scaled co-ordinates */
void blockClear(int x, int y){
	int i = 0;
	int j = 0;
	
	//scaling the co-ordinates
	x = x*SCALE_FACTOR;
	y = y*SCALE_FACTOR;
	
	//printing the block
	for(i=x;i<(x+SCALE_FACTOR-1);i++) {
		for(j=y;j<(y+SCALE_FACTOR-1);j++) {
			GLCD_RemovePixel(i, j);
		}
	}
}

void Tankmove(void) {
	//VARIABLES
	uint32_t buffer = 0;
	int tankX = 0;
	int tankY = 0;
	
	//Read from GPIO1
	buffer |= LPC_GPIO1->FIOPIN;
	
	//UP
	if((buffer & BIT23) == 0) {
		blockClear(tankX,tankY);
		tankY = tankY+1;
		blockPrint(tankX,tankY);
		
  }
	//Right
	else if((buffer & BIT24) == 0) {
		blockClear(tankX,tankY);
		tankY = tankY+1;
		blockPrint(tankX,tankY);
	}
	//Down
	else if((buffer & BIT25) == 0) {
		blockClear(tankX,tankY);
		tankY = tankY+1;
		blockPrint(tankX,tankY);
	}
	//Left
	else if((buffer & BIT26) == 0) {
		blockClear(tankX,tankY);
		tankY = tankY+1;
		blockPrint(tankX,tankY);
	}
	else
		printf("NO DIR");
	printf("\r");
	
	if((buffer & BIT20) == 0)
		printf("Pressed");
	else
		printf("Not Pressed");
	printf("\r\r");
}

void mapPrint(void) {
	//Variables
	int i=0;
	int j=0;
	uint16_t column = 0;
	
	for(i=0;i<20;i++) { //20 columns in a 1:16 scale
		column = map[i];
		
		for(j=0;j<15;j++) {
			if(column & (0x1 << (15-j))) //check if area should be occupied
				blockPrint(i,j);
		}
	}
}


void lcdInit() {
	GLCD_Init();
	GLCD_Clear(Blue);
	GLCD_SetBackColor(Blue);
	GLCD_SetTextColor(White);
}

void initialization() {
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
	LPC_GPIO1->FIOCLR |= LED_ALL_G1;
	LPC_GPIO2->FIOCLR |= LED_ALL_G2;
	
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
//	int i=0;
	initialization();
	
	mapPrint();
	while(1) {}
}