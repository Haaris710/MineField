#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <rtl.h>
#include "GLCD.h"

// Bit Masks
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
#define LED_ALL_G1 (BIT28 | BIT29 | BIT31)
#define LED_ALL_G2 (BIT2 | BIT3 | BIT4 | BIT5 | BIT6)

// Improves readability
#define TIMEOUT_INDEFINITE (0xffff)
#define ONE_SECOND (200)

// SYNCHRONIZATION VARIABLES //
OS_MUT lcdMtx;
OS_MUT tankDataMtx;
OS_MUT mineDataMtx;

// FUNCTION PROTOTYPES //
__task void mines_task(void);
__task void tank_task(void);
__task void display_task(void);
__task void joy_task(void);

//////////////////////////////////////////////////////////////////////////
//												GAME CHARACTERISTICS													//
//////////////////////////////////////////////////////////////////////////

struct controlCharacs {
	uint8_t joyStick[5];
	uint8_t pushBtn;
};
struct controlCharacs controls;

struct mapCharacs {
	uint8_t scaleFactor;
	uint16_t mapBackColor;
	uint16_t mapBlockColor;
	
	//tank color characteristics
	uint16_t tankBodyColor;
	uint16_t tankNoseColor;
	
	//mine characteristics
	uint16_t minesCycleSpeed;
	
	//mine color characteristics
	uint16_t minesInvis;
	uint16_t minesPrimed;
	uint16_t minesExp;
	
	//test mine colors
	uint16_t minesGreen;
	uint16_t minesBlue;
	uint16_t minesYellow;
	uint16_t minesRed;
};
struct mapCharacs map;

// Unscoped enum type
typedef enum Directions { 
	NO_DIR = 0,
	UP = 1,
	RIGHT = 2,
	LEFT = 3,
	DOWN = 4
}Directions;

struct tankCharacs {
	Directions dirCur;
	Directions dirNext;
	uint8_t isMoving;
	uint8_t xCur;
	uint8_t yCur;
	uint8_t xNext;
	uint8_t yNext;
};
struct tankCharacs tank;

/*
	Map defined as an array of bit maps (global constant)
	
	Map consists of 10, 32-bit numbers representing the vertical
	columns of pixels on a 1:16 scale. Each bit reprents a pixel
	as occupied or unoccupied.
*/
static const uint32_t mapBitField[20] = {
	0,0,0,0x19F0,0x19F0,0x8000,0x8000,0x1F00,0x1F26,0x1F26,0x0006,0x0006,
	0xF986,0xF986,0x0186,0x0180,0x3990,0x3818,0,0
};

typedef enum MineState {
	INVIS = 0,
	PRIMED = 1,
	EXP = 2,
	
	//Test states
	T1 = 3,
	T2 = 4,
	T3 = 5,
	T4 = 6
} MineState;

// Array of four mine sets (0 to 3)
static MineState minesCur[4];
static MineState minesNext[4];

static const uint8_t mineSet1X[51] = {
	0,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,3,4,5,5,5,6,6,7,7,8,8,9,10,10,10,11,11,
	11,11,12,12,14,14,15,15,15,15,17,17,18,18,18,19,19,19
};
static const uint8_t mineSet1Y[51] = {
	0,4,7,9,10,3,11,12,13,1,5,6,8,9,13,14,2,13,2,11,13,6,9,1,13,9,12,11,1,
	4,7,4,6,8,10,10,11,3,4,1,5,9,12,6,14,0,8,11,3,10,14
};

static const uint8_t mineSet2X[56] = {
	0,0,0,0,0,0,1,1,1,1,1,2,2,3,3,3,3,3,4,4,5,5,5,6,6,6,6,7,8,8,10,10,10,11,
	11,11,12,12,13,13,13,14,14,15,15,16,16,17,18,18,18,18,18,19,19,19
};
static const uint8_t mineSet2Y[56] = {
	1,3,5,6,12,13,1,2,7,8,10,4,11,0,5,12,13,14,1,6,3,7,10,3,5,11,12,8,0,2,6,
	10,12,1,3,7,9,12,5,9,10,2,11,3,14,5,13,0,2,6,9,10,14,6,8,12
};

static const uint8_t mineSet3X[58] = {
	0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,4,4,5,5,5,5,5,6,6,6,7,8,9,9,10,10,10,
	10,11,11,11,13,13,14,14,14,15,15,15,16,17,17,17,18,18,18,18,19,19,19
};
static const uint8_t mineSet3Y[58] = {
	2,8,11,14,0,4,5,6,9,14,0,2,3,7,10,12,1,6,2,5,1,4,5,8,12,2,13,14,9,11,0,9,
	3,5,8,11,2,9,11,11,12,0,6,10,2,4,11,10,1,5,8,4,7,12,13,1,5,13
};

static const uint8_t mineSet4X[59] = {
	4,4,4,5,5,5,6,6,6,6,6,7,7,7,7,7,7,8,8,9,9,9,9,10,10,10,11,11,11,12,12,13,
	14,14,14,14,15,15,15,15,16,16,16,16,16,16,17,17,17,17,18,18,18,19,19,19,
	19,19,19
};
static const uint8_t mineSet4Y[59] = {
	0,12,14,6,9,14,1,4,7,8,10,0,2,10,11,12,14,1,8,1,2,8,12,0,2,9,0,5,12,5,6,6,
	1,5,9,12,0,6,10,13,0,1,6,9,12,14,7,9,10,13,1,3,5,0,2,4,7,9,11
};



//////////////////////////////////////////////////////////////////////////
//											INITIALIZATION FUNCTIONS												//
//////////////////////////////////////////////////////////////////////////

void ledInit() {
	//set LEDs to output
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	//clear all LEDs
	LPC_GPIO1->FIOCLR |= LED_ALL_G1;
	LPC_GPIO2->FIOCLR |= LED_ALL_G2;
}

void lcdInit() {
	GLCD_Init();
	GLCD_Clear(map.mapBackColor);
	GLCD_SetBackColor(map.mapBackColor);
}

void mapCharacsInit() {
	//map characteristics
	map.scaleFactor = 16;
	map.mapBackColor = Black;
	map.mapBlockColor = Magenta;
	
	//tank color characteristics
	map.tankBodyColor = Olive;
	map.tankNoseColor = White;
	
	//mine characteristics
	map.minesCycleSpeed = ONE_SECOND*2;
	
	//mine color characteristics
	map.minesInvis = Black;
	map.minesPrimed = Yellow;
	map.minesExp = Red;
	
	//test colors for mines
	map.minesGreen = Green;
	map.minesBlue = Blue;
	map.minesYellow = Yellow;
	map.minesRed = Red;
}

void tankCharacsInit() {
	tank.dirCur = UP;
	tank.dirNext = UP;
	tank.isMoving = 0;
	tank.xCur = 0;
	tank.yCur = 14;
	tank.xNext = 0;
	tank.yNext = 0;
}

void mineStatesInit() {
	//counter variable
	int i=0;
	
	//initalize all mine sets to invisible state
	for(i=0; i<4; i++) {
		minesCur[i] = INVIS;
		minesNext[i] = INVIS;
	}
}

void pushButtonInit() {
	
	//set push button connected to GPIO
	LPC_PINCON->PINSEL4 &= ~(3<<20);
	//set as input
	LPC_GPIO2->FIODIR &= ~(1<<10);
	//setup read on falling edge interrupt
	LPC_GPIOINT->IO2IntEnF |= (1<<10);
	//enable IRQ
	NVIC_EnableIRQ(EINT3_IRQn);
}

//ISR for push button
void EINT3_IRQHandler(void) {
	//set push button 'state' to 'pushed'
	tank.isMoving = 0;
	
	//clear interrupt
	LPC_GPIOINT->IO2IntClr |= (1<<10);
}

void initialization() {
	SystemInit();
	ledInit();
	lcdInit();
	mapCharacsInit();
	tankCharacsInit();
	pushButtonInit();
}

////////////////////////////////////////////////////////////////////////////
//														PRINT FUNCTIONS															//
////////////////////////////////////////////////////////////////////////////

/*Prints a 16 pixel square block with parameters 
  as X and Y which represent scaled co-ordinates */
void blockPrint(int x, int y){
	int i = 0;
	int j = 0;
	
	//set block color
	GLCD_SetTextColor(map.mapBlockColor);
	
	//scale the coordinates
	x = x*(map.scaleFactor);
	y = y*(map.scaleFactor);
	
	//print the block
	for(i=x;i<(x+(map.scaleFactor)-1);i++) {
		for(j=y;j<(y+(map.scaleFactor)-1);j++) {
			GLCD_PutPixel(i, j);
		}
	}
}

/*Prints a 16 pixel mine with parameters 
  as X and Y which represent scaled co-ordinates */
void minePrint(int x, int y){
	int i = 0;
	int j = 0;
	
	//scale the coordinates
	x = x*(map.scaleFactor) + (map.scaleFactor/2);
	y = y*(map.scaleFactor) + (map.scaleFactor/2);
	
	//print the mine
	for (i=8;i >= -8;i-=2)  {	
			for (j=-8;j <= 8;j++)  {
				
				if(i*i+j*j == 16)
				{
					GLCD_PutPixel(i+x, j+y);
					GLCD_PutPixel(i+x+1, j+y);
					GLCD_PutPixel(i+x, j+y+1);
					GLCD_PutPixel(i+x-1, j+y);
					GLCD_PutPixel(i+x, j+y-1);
				}
					
			 }
  }
}


//Prints a mine set in a given state
void mineSetPrint(uint8_t setNum, MineState mState) {
	//variables
	int i = 0;
	int j = 0;
	int needReprint = 0;
	
	//check for valid input
	if(setNum < 0 || setNum > 3)
		return;
	
	//check if re-print required
	if(minesNext[setNum] != minesCur[setNum])
		needReprint = 1;
	if(!needReprint)
		return;
	
	//store mine state in global array
	minesCur[setNum] = mState;
	
	switch(mState) {
		case INVIS:
			GLCD_SetTextColor(map.minesInvis);
			break;
		case PRIMED:
			GLCD_SetTextColor(map.minesPrimed);
			break;
		case EXP:
			GLCD_SetTextColor(map.minesExp);
			break;
		case T1:
			GLCD_SetTextColor(map.minesGreen);
			break;
		case T2:
			GLCD_SetTextColor(map.minesBlue);
			break;
		case T3:
			GLCD_SetTextColor(map.minesYellow);
			break;
		case T4:
			GLCD_SetTextColor(map.minesRed);
	}
	
	if(setNum == 0) {
		for(i=0; i<51; i++) {
			minePrint(mineSet1X[i],mineSet1Y[i]);
		}
	}
	else if(setNum == 1) {
		for(i=0; i<56; i++) {
			minePrint(mineSet2X[i],mineSet2Y[i]);
		}
	}
	else if(setNum == 2) {
		for(i=0; i<58; i++) {
			minePrint(mineSet3X[i],mineSet3Y[i]);
		}
	}
	else if(setNum == 3) {
		for(i=0; i<59; i++) {
			minePrint(mineSet4X[i],mineSet4Y[i]);
		}
	}
}


/*Clears a 16 pixel square block with parameters 
  as X and Y which represent scaled co-ordinates */
void blockClear(int x, int y){
	int i = 0;
	int j = 0;
	
	//scaling the co-ordinates
	x = x*(map.scaleFactor);
	y = y*(map.scaleFactor);
	
	//clearing the block
	for(i=x;i<(x+(map.scaleFactor));i++) {
		for(j=y;j<(y+(map.scaleFactor));j++) {
			GLCD_RemovePixel(i, j);
		}
	}
}


void tankPrint(int x, int y, Directions dir) {
	int i=0;
	int j=0;
	
	//scale coordinates
	x = x*(map.scaleFactor);
	y = y*(map.scaleFactor);
	
	//print tank body
	GLCD_SetTextColor(map.tankBodyColor);
	if(dir == LEFT) {
		for(i=x;i<(x+(map.scaleFactor)-1);i++) {
			for(j=(y+((map.scaleFactor)/4)); j<(y+(map.scaleFactor)-1); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	else if(dir == RIGHT) {
		for(i=x; i<x+((map.scaleFactor)-1); i++) {
			for(j=y; j<y+(((map.scaleFactor)/4)*3); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	else if(dir == UP) {
		for(i=x; i<x+(((map.scaleFactor)/4)*3); i++) {
			for(j=y; j<y+(map.scaleFactor); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	else if(dir == DOWN) {
		for(i=x+((map.scaleFactor)/4); i<x+(map.scaleFactor); i++) {
			for(j=y; j<y+(map.scaleFactor); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	
	//print tank nose
	GLCD_SetTextColor(map.tankNoseColor);

	if(dir == LEFT) {
		for(i=(x+((map.scaleFactor)/4)); i<(x+(((map.scaleFactor)/4)*3)); i++) {
			for(j=y; j<(y+4);j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	else if(dir == RIGHT) {
		for(i=(x+((map.scaleFactor)/4)); i<(x+(((map.scaleFactor)/4)*3)); i++) {
			for(j=y+(((map.scaleFactor)/4)*3); j<y+(map.scaleFactor); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	else if(dir == UP) {
		for(i=x+(((map.scaleFactor)/4)*3); i<x+(map.scaleFactor); i++) {
			for(j=y+((map.scaleFactor)/4); j<y+((((map.scaleFactor)/4)*3)); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
	else if(dir == DOWN) {
		for(i=x; i<x+((map.scaleFactor)/4); i++) {
			for(j=y+((map.scaleFactor)/4); j<y+((((map.scaleFactor)/4)*3)); j++) {
				GLCD_PutPixel(i,j);
			}
		}
	}
}

/*
//0 is up, 1 is right, 2 is left and 3 is down
void tankMove(int tankX, int tankY, Directions dir) {
	//VARIABLES
	uint32_t buffer = 0;
	int isMoving = 0;
	
	while(1){
			
		//If Joy stick is pressed the tank moves is the direction its pointing to
		buffer = 0;
		buffer |= LPC_GPIO1->FIOPIN;
		if((buffer & BIT20) == 0)
			isMoving = 1;
		
		//If the tank is stationary the tank is allowed to change direction
		if(isMoving == 0){
			
			buffer = 0;	
			buffer |= LPC_GPIO1->FIOPIN;
			//Left
			if((buffer & BIT23) == 0) {
				dir = LEFT;
			}
			//Up 
			else if((buffer & BIT24) == 0) {
				dir = UP;	
			}
			//Right
			else if((buffer & BIT25) == 0) {
				dir = RIGHT;
			}
			//Down
			else if((buffer & BIT26) == 0) {
				dir = DOWN;
			}
			else {
				printf("Error");
			}
		}
			
		//Read from GPIO1
		while(isMoving){
		
			buffer = 0;	
			buffer |= LPC_GPIO1->FIOPIN;
			//Left
			if(dir == LEFT) {
				blockClear(tankX,tankY);
				tankY = tankY-1;
				tankPrint(tankX,tankY,LEFT);	
			}
			//Up 
			else if(dir == UP) {
				blockClear(tankX,tankY);
				tankX = tankX+1;
				tankPrint(tankX,tankY,UP);	
			}
			//Right
			else if(dir == RIGHT) {
				blockClear(tankX,tankY);
				tankY = tankY+1;
				tankPrint(tankX,tankY,RIGHT);		
			}
			//Down
			else if (dir == DOWN) {
				blockClear(tankX,tankY);
				tankX = tankX-1;
				tankPrint(tankX,tankY,DOWN);	
			}
			else {
				printf("Error");
			}
			
			//If stop button is pressed, the tank stops
			buffer =0;
			buffer |= LPC_GPIO2->FIOPIN;
			if((buffer & BIT10) == 0)
				isMoving = 0;
				
		}
	}	
}
*/

void mapPrint(void) {
	//Variables
	int i=0;
	int j=0;
	uint16_t column = 0;
	
	for(i=0;i<20;i++) { //20 columns in a 1:16 scale
		column = mapBitField[i];
		
		for(j=0;j<15;j++) {
			if(column & (0x1 << (15-j))) //check if area should be occupied
				blockPrint(i,j);
		}
	}
}

// PERIPHERAL FUNCTIONS //

void pushBtnRead(void) {
	//VARIABLES
	uint32_t buffer = 0;
	
	//Read from GPIO2
	buffer |= LPC_GPIO2->FIOPIN;
	
	if((buffer & BIT10) == 0) {
		os_mut_wait(tankDataMtx, TIMEOUT_INDEFINITE);
		tank.isMoving = 0;
		os_mut_release(tankDataMtx);
	}
}

/*
	joyStickRead function returns a uint8_t bit vector which signify
	direction:
	0000 0001 LEFT = 0x01
	0000 0100 UP = 0x04
	0000 0010	RIGHT = 0x02
	0000 0011 DOWN = 0x03
	
	BIT4 indicates pressed or unpressed
*/
uint8_t joyStickRead(void) {
	//VARIABLES
	uint32_t buffer = 0;
	uint8_t output = 0;
	
	//Read from GPIO1
	buffer |= LPC_GPIO1->FIOPIN;
	
	if((buffer & BIT23) == 0) { //left
		output = 0x01;
  }
	else if((buffer & BIT24) == 0) { //up
		output = 0x04;
	}
	else if((buffer & BIT25) == 0) { //right
		output = 0x02;
	}
	else if((buffer & BIT26) == 0) { //down
		output = 0x03;
	}
	else
		output = 0;
	
	if((buffer & BIT20) == 0) //pressed
		output |= (0x01 << 4);
	
	return output;
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

//initializes all tasks and then deletes self
__task void init_tasks(void) {
	//initialize mutexes and semaphores
	os_mut_init(&lcdMtx);
	os_mut_init(&mineDataMtx);
	
	//initialize tasks
	os_tsk_create(mines_task,1);
	//os_tsk_create(tank_task,1);
	os_tsk_create(joy_task,1);
	os_tsk_create(display_task,1);
	
	//init_tasks self delete
	os_tsk_delete_self();
}

//updates tank position data
__task void tank_task(void) {
	while(1) {
		if(tank.isMoving) {
			os_mut_wait(&tankDataMtx, TIMEOUT_INDEFINITE);
			switch(tank.dirCur) {
				case(LEFT):
					tank.yNext = tank.yCur-1;
					break;
				case(UP):
					tank.xNext = tank.xCur+1;
					break;
				case(RIGHT):
					tank.yNext = tank.yCur+1;
					break;
				case(DOWN):
					tank.xNext = tank.xCur;
			}
			os_mut_release(&tankDataMtx);
		}
	}
}

//updates mine set states
__task void mines_task(void) {
	//mine state tracking variable
	int i = 0;
	os_itv_set(map.minesCycleSpeed);
	
	while(1) {
		//change state of current mine set to PRIMED
		os_mut_wait(&mineDataMtx, TIMEOUT_INDEFINITE);
		minesNext[i] = PRIMED;
		os_mut_release(&mineDataMtx);
		os_itv_wait();
		
		//change state of current mine set to EXP
		os_mut_wait(&mineDataMtx, TIMEOUT_INDEFINITE);
		minesNext[i] = EXP;
		os_mut_release(&mineDataMtx);
		os_itv_wait();
		
		//change state of current mine set to INVIS
		//change state of next mine set to PRIMED
		os_mut_wait(&mineDataMtx, TIMEOUT_INDEFINITE);
		
		minesNext[i] = INVIS;
		if(i==3) minesNext[0] = PRIMED;
			else minesNext[i+1] = PRIMED;
		
		os_mut_release(&mineDataMtx);
		os_itv_wait();
		
		//increment state counter
		if(i==3) i=0;
			else i++;
	}
}

__task void joy_task(void) {
	uint8_t state = 0;
	char* output;
	
	while(1) {
		os_mut_wait(&tankDataMtx,TIMEOUT_INDEFINITE);
		if(tank.isMoving == 0) {
			state = joyStickRead();
			if((state & 0x07) == 0x01) {
				tank.dirNext = LEFT;
			}
			else if((state & 0x07) == 0x02) {
				tank.dirNext = RIGHT;
			}
			else if((state & 0x07) == 0x03) {
				tank.dirNext = DOWN;
			}
			else if((state & 0x07) == 0x04) {
				tank.dirNext = UP;
			}
			
			if((state & (0x01 << 4)) > 0)
				tank.isMoving = 1;
			
			os_mut_release(&tankDataMtx);
			
			os_tsk_pass();
		}
		else {
			//VARIABLES
			uint32_t buffer = 0;
	
			//Read from GPIO2
			buffer |= LPC_GPIO2->FIOPIN;
	
			if((buffer & BIT10) == 0) {
				tank.isMoving = 0;
			}
		}
	}
}


//Uses tank data and mine data to print display
__task void display_task(void) {
	int i=0;
	
	//print tank starting position
	os_mut_wait(tankDataMtx, TIMEOUT_INDEFINITE);
	tankPrint(tank.xCur, tank.yCur, tank.dirCur);
	os_mut_release(tankDataMtx);
	
	while(1) {
		
		//print mine sets if changed
		for(i=0; i<4; i++) {
			os_mut_wait(&mineDataMtx, TIMEOUT_INDEFINITE);
			mineSetPrint(i,minesNext[i]);
			os_mut_release(&mineDataMtx);
		}
		
		//print tank direction if changed
		if(tank.dirNext != tank.dirCur) {
			os_mut_wait(&tankDataMtx, TIMEOUT_INDEFINITE);
			tank.dirCur = tank.dirNext;
			blockClear(tank.xCur,tank.yCur);
			tankPrint(tank.xCur,tank.yCur,tank.dirNext);
			os_mut_release(&tankDataMtx);
		}
		
		
	}
}

int main(void) {
	initialization();
	mapPrint();
	printf("test");
	os_sys_init(init_tasks);
	
	/*
	mapPrint();
	mineSetPrint(0, T1);
	mineSetPrint(1, T2);
	mineSetPrint(2, T3);
	mineSetPrint(3, T4);
	*/
	//tankPrint(0,0,LEFT);
	//tankMove(0,0, LEFT);
	/*
	tankPrint(0,2,DOWN);
	tankPrint(0,4,LEFT);
	tankPrint(0,6,RIGHT);
	*/
	//while(1) {}
}