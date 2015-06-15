//*****************************************************************************
//   임베디드 프로세서 응용 - 이종원 교수님
//  박강규
//  박고언
//  김대웅
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ints.h"
#include "inc/hw_i2c.h"
#include "inc/hw_types.h"

#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/debug.h"
#include "driverlib/i2c.h"


#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "drivers/frame.h"
#include "drivers/kentec320x240x16_ssd2119.h"
#include "drivers/pinout.h"
#include "drivers/touch.h"

#define GROUNDBOUND 170
#define UPBOUND 140
#define INIT_LOCATION 350

void Timer0AIntHandler(void); // For GPTM INT

int j;
float obstacle_X = 0;
int timenum=0;
int DpointX = 120, DpointY = 200; // 공룡의 x좌표와 y좌표
int OpointX = 350, OpointY = 180; // 장애물의 x좌표와 y좌표
//*****************************************************************************
//
// Google Dinosaur Game
//
//*****************************************************************************

//*****************************************************************************
//
// A global we use to keep track of when the user presses the screen.
//
//*****************************************************************************
volatile bool touched = false;
int state=0;
tContext sContext;
//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif

//*****************************************************************************
//
// This function is called in interrupt context by the touch screen driver when
// there is a pointer event.
//
//*****************************************************************************
int32_t
TSHandler(uint32_t ui32Message, int32_t i32X, int32_t i32Y)
{
    //
    // See if this is a pointer up message.
    //
    if(ui32Message == WIDGET_MSG_PTR_UP)
    {
        //
        // The screen has been pressed and released
        //
        touched = true;
        state = 1;
    }

    //
    // Success.
    //
    return(0);
}


int dinosaur[8][10] = {
     {0,0,0,0,0,1,1,1,0,0},
     {0,0,0,0,1,1,0,1,1,0},
     {0,0,0,0,1,1,1,1,1,0},
     {1,0,0,0,1,1,1,1,0,0},
     {1,1,0,0,1,1,1,1,1,1},
     {1,1,0,0,0,1,1,1,0,0},
     {0,1,1,1,1,1,1,1,0,0},
     {0,0,0,0,1,1,0,0,0,0}
};



void checkCrash()
{
   if((DpointX <= OpointX && OpointX <= DpointX + 15) && (OpointY <= DpointY && DpointY <= OpointY + 20))
   {
      GrContextFontSet(&sContext, g_psFontCm20);
      GrStringDrawCentered(&sContext, "Game Over", -1,
      GrContextDpyWidthGet(&sContext) / 2,
      GrContextDpyHeightGet(&sContext) / 10 + 50, 0);
      SysCtlDelay(120000000);
   }
}

//Initial Map
int map[100] = {50, 100, 160, 220, 270, 350, 400, 450, 510, 560,
				660, 710, 760, 820, 900, 950, 1000, 1050, 1110, 1160,
				1220, 1270, 1350, 1400, 1450, 1510, 1590, 1650, 1700,
				1820, 1970, 2020, 2090, 2150, 2200, 2270, 2350, 2400,
				2450, 2500, 2550, 2600, 2670, 2740, 2850, 2900, 3000,
				3050, 3110, 3180, 3260, 3310, 3360, 3420, 3500, 3550,
				4000, 4050, 4110, 4180, 4250, 4300, 4350, 4400, 4450,
				4500, 4560, 4570, 4630, 4700, 4750, 4800, 4860, 4930,
				5000, 5050, 5120, 5170, 5230, 5300, 5350, 5400, 5460,
				5510, 5570, 5630, 5700, 5760, 5820, 5900, 5950, 6000
};
int move=0;

void ChangeMap(){
	int i = 0;
	map[i] = 50;	//The first obstacle
	srand (time(NULL));
	for(i = 1; i<100; i++){
		map[i] = map[i-1] + rand()%50 + 50;	//obstacle = previous + 50~100
	}
}

void drawMap(){
	int k;
	 GrContextForegroundSet(&sContext, ClrSilver);
	 for(k = 0; k < 100; k++){
		GrStringDraw(&sContext, "****", -1, map[k] - move, 190, 0);
		GrStringDraw(&sContext, "****", -1, map[k] - move, 192, 0);
		GrStringDraw(&sContext, "****", -1, map[k] - move, 194, 0);
	 }
}

void clear()
{
   SysCtlDelay(100000);
   int i;
   GrFlush(&sContext);
   GrContextForegroundSet(&sContext, ClrBlack);
   for(i = 100; i < 200; i++){ //for LCD Background Reset
      GrContextFontSet(&sContext, g_psFontCm12);
      GrLineDraw(&sContext, 0, i, 380, i);
   }
}
void drawDinosaur(float y2)
{

   int i, j, x;
   int y;
   GrContextForegroundSet(&sContext, ClrGreen);   // 파란색으로
   for(i = 0,y = y2; i < 8; i++, y += 3){
      x = 120;
     for(j = 0; j < 10; j++){
        if(dinosaur[i][j] == 1){
           GrContextFontSet(&sContext, g_psFontCm12);
           GrStringDraw(&sContext, "-", -1, x, y, 0);

        }
        x += 2.5;
     }
   }
}


void jump()
{
   int y = GROUNDBOUND;
   while(1){
      drawDinosaur(y);
      drawMap();
      clear();
      y -= 3;
      move += 2;
      DpointY = y;
	   for(j = 0; j < 100; j++){
		  OpointX = map[j] - move;
		  checkCrash();
		  if(135 < OpointX)
			  break;
	   }
//      DpointY = y;
//      OpointX = obstacle_X;
//      checkCrash();
//      if(obstacle_X < 0) obstacle_X = INIT_LOCATION;
      if(y <= UPBOUND) break;
    }

     while(1){
      drawDinosaur(y);
      drawMap();
      clear();
      y += 3;
      move += 2;
      DpointY = y;
      for(j = 0; j < 100; j++){
		  OpointX = map[j] - move;
		  checkCrash();
		  if(135 < OpointX)
			  break;

	   }
//      DpointY = y;
//     OpointX = obstacle_X;
//      checkCrash();
//      if(obstacle_X < 0) obstacle_X = INIT_LOCATION;
      if(y >= GROUNDBOUND) break;
     }
}



///////////////itoa function//////////////////////////////////////////////////////////////////////////////////////
void itoa(int num, char *pStr){
    int radix = 10;
    int deg = 1;
    int cnt = 0;
    int i;

    if(pStr == NULL) return ;

    if(num < 0){
        *pStr = '-';
        num *= -1;
        pStr++;
    }

    while(1){
        if( (num / deg) > 0)
            cnt++;
        else
            break;

        deg *= radix;
    }
    deg /= radix;

    for(i = 0; i < cnt; i++, pStr++){
        *pStr    = (num / deg) + '0';
        num        -= (num / deg) * deg;
        deg        /= radix;
    }
    *pStr = '\0';
}
////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	uint32_t ui32SysClock;
	uint32_t ulIdy;

	ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
										   SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
										   SYSCTL_CFG_VCO_480), 120000000);

//LCD

	PinoutSet();
	Kentec320x240x16_SSD2119Init(ui32SysClock);
	GrContextInit(&sContext, &g_sKentec320x240x16_SSD2119);


//Timer

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

   TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
   TimerLoadSet(TIMER0_BASE, TIMER_A, 120000000-1);

   //I2CMasterIntEnableEx(I2C6_BASE, I2C_SLAVE_INT_DATA);
   TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

   //IntEnable(INT_I2C6);
   IntEnable(INT_TIMER0A);
   IntMasterEnable();

   TimerEnable(TIMER0_BASE, TIMER_A);



//Touch Screen

	TouchScreenInit(ui32SysClock);

	TouchScreenCallbackSet(TSHandler);


//Background
	GrContextForegroundSet(&sContext,
				(((((240 - ulIdy) * 255) / 240) <<ClrRedShift) |
				 (((ulIdy * 255) /240) <<ClrGreenShift)));
	 GrLineDraw(&sContext, 0, 5, 319, 5);

	 GrLineDraw(&sContext, 0, 200, 319, 200);

	 GrContextForegroundSet(&sContext, ClrBlue);
	 GrContextFontSet(&sContext, g_psFontCm20);
	 GrStringDrawCentered(&sContext, "Google Dinosaur Game!", -1,
				GrContextDpyWidthGet(&sContext) / 2,
				GrContextDpyHeightGet(&sContext) / 10, 0);

	 GrContextForegroundSet(&sContext, ClrRed);
	 GrContextFontSet(&sContext, g_psFontCm12);
	 GrStringDraw(&sContext, "Record : ", -1, 165, 55, 0);
	 GrContextForegroundSet(&sContext, ClrGold);
	 GrStringDraw(&sContext, "280", -1, 220, 55, 0);

	 GrContextForegroundSet(&sContext, ClrRed);
	 GrContextFontSet(&sContext, g_psFontCm12);
	 GrStringDraw(&sContext, "Score : ", -1, 250, 55, 0);

	 GrContextForegroundSet(&sContext, ClrBlue);
	 GrContextFontSet(&sContext, g_psFontCm12);
	 GrStringDrawCentered(&sContext, "===Progress===", -1,
					 GrContextDpyWidthGet(&sContext) / 2,
					 GrContextDpyHeightGet(&sContext) *9 / 10, 0);

//Map
	 ChangeMap();

     //Flush any cached drawing operation
     GrFlush(&sContext);

    obstacle_X = INIT_LOCATION;

    while(1)
    {

       while(!touched) // touch 하기 전 공룡은 가만히 있고 장애물만 왼쪽으로 계속 이동
       {
    	  drawDinosaur(GROUNDBOUND);
		  drawMap();
		  clear();
		  move += 2;
		   for(j = 0; j < 100; j++){
			  OpointX = map[j] - move;
			  checkCrash();
			  if(135 < OpointX)
				  break;
		   }
   //      OpointX = obstacle_X;
   //      checkCrash();
   //      if(obstacle_X < 0) obstacle_X = INIT_LOCATION;
       }

       if(state)
       {
          jump();
		  state=0;
		  touched = false;
       }

    }
}

void Timer0AIntHandler(void)
{
	int i=0;
	char time[20] = {0};
	int temp = 0;
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	GrContextForegroundSet(&sContext, ClrBlack);
	for (i = 0; i<50; i++) //for LCD Background Reset
		GrLineDraw(&sContext, 290, 50 + i, 360, 50 + i);


	GrContextForegroundSet(&sContext, ClrSilver);
	timenum++;
	temp = timenum;
	itoa(temp, time);
	GrStringDraw(&sContext, time, -1, 300, 55, 0);
}
