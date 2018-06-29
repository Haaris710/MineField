// LED support

// Written by Bernie Roehl, February 2017

#include <lpc17xx.h>
#include "LED.h"

void LED_setup(void) {
	LPC_GPIO1->FIODIR |= 0xB0000000;    // port 1
	LPC_GPIO2->FIODIR |= 0x0000007C;    // port 2
	LPC_GPIO1->FIOCLR = 0xB0000000;
	LPC_GPIO2->FIOCLR = 0x0000007C;
}

const unsigned char led_pos[8] = { 28, 29, 31, 2, 3, 4, 5, 6 };

void LED_set(int n) {
	int mask = 1 << led_pos[n];
	if (n < 3) LPC_GPIO1->FIOSET = mask;
	else LPC_GPIO2->FIOSET = mask;
}

void LED_clear(int n) {
	int mask = 1 << led_pos[n];
	if (n < 3) LPC_GPIO1->FIOCLR = mask;
	else LPC_GPIO2->FIOCLR = mask;
}

void LED_display(unsigned char n) {
	int i;
	for (i = 0; i < 8; ++i)
		if (n & (1 << i))
			LED_set(i);
		else
			LED_clear(i);
}
