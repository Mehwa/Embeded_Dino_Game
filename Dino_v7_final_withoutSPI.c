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
#define UPBOUND 130
#define INIT_LOCATION 350
#define MAPSIZE 300

void Timer0AIntHandler(void); // For GPTM INT

int j;
int ObstacleIndex = 0;
int timenum=0;
int DpointX = 120, DpointY = 200; // 공룡의 x좌표와 y좌표
int OpointX = 350, OpointY = 180; // 장애물의 x좌표와 y좌표
int color = 0;   //dinosaur color ( 0 : GOld, 1 : Red, 2 : DeepPink, 3 : ClrGainsboro)
float speed = 2;

// TimerInt
int i0=0;
char timer0[20] = {0};

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

int arrow[6][10] = {
    {0,0,1,1,0,0,0,0,0,0},
    {0,1,1,1,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1},
    {0,1,1,1,0,0,0,0,0,0},
    {0,0,1,1,0,0,0,0,0,0}
};


void checkCrash()
{
   if((45 <= OpointX && OpointX <= 70) && (193 <= DpointY && DpointY <= 210))
   {
     GrContextForegroundSet(&sContext, ClrSilver);
      GrContextFontSet(&sContext, g_psFontCm20);
      GrStringDrawCentered(&sContext, "Game Over", -1,
            GrContextDpyWidthGet(&sContext) / 2,
           GrContextDpyHeightGet(&sContext) / 10 + 50, 0);
      IntMasterDisable();
      exit(1);
   }
}

//Initial Map
int map[MAPSIZE] = {0};

void ChangeMap(){
   int i = 0;
   map[i] = 200;   //The first obstacle
   srand (time(NULL));
   for(i = 1; i < MAPSIZE; i++){
      map[i] = map[i-1] + rand()%50 + 65;   //obstacle = previous + 50~100
   }
}

void drawMap(){
   int k;
    GrContextForegroundSet(&sContext, ClrSilver);
    for(k = 0; k < MAPSIZE; k++){
      GrStringDraw(&sContext, "****", -1, map[k], 190, 0);
      GrStringDraw(&sContext, "****", -1, map[k], 192, 0);
      GrStringDraw(&sContext, "****", -1, map[k], 194, 0);
    }
}

void clear()
{
   int i;
   //GrFlush(&sContext);
   GrContextForegroundSet(&sContext, ClrBlack);
   for(i = 130; i < 200; i++)
      GrLineDraw(&sContext, 0, i, 330, i);
}

void drawDinosaur(float y2)
{

   int i, j, x;
   int y;
   if(color == 0)
      GrContextForegroundSet(&sContext, ClrGold);
   else if(color == 1)
      GrContextForegroundSet(&sContext, ClrRed);
   else if(color == 2)
      GrContextForegroundSet(&sContext, ClrDeepPink);
   else if(color == 3)
      GrContextForegroundSet(&sContext, ClrAqua);

   for(i = 0,y = y2; i < 8; i++, y += 3){
      x = 50;
     for(j = 0; j < 10; j++){
        if(dinosaur[i][j] == 1){
           GrContextFontSet(&sContext, g_psFontCm12);
           GrStringDraw(&sContext, "-", -1, x, y, 0);

        }
        x += 2.5;
     }
   }
}

void drawArrow(int x, int y){
   int i,j;
   int tempx = x;
   GrContextForegroundSet(&sContext, ClrSilver);
   for(i = 0; i < 6; i++, y+=3){
        for(j = 0, x=tempx; j < 10; j++, x+=3){
           if(arrow[i][j] == 1){
              GrContextFontSet(&sContext, g_psFontCm12);
              GrStringDraw(&sContext, "-", -1, x, y, 0);
           }
        }
   }
}
void deleteArrow(int x, int y){
   int i;
   GrFlush(&sContext);
   GrContextForegroundSet(&sContext, ClrBlack);
   for(i = y; i < y+25; i++)
      GrLineDraw(&sContext, x, i, x+30, i);
}

void flymode(int x)
{
   speed = 4;
   while(x + 5 != ObstacleIndex){
      drawDinosaur(130);
       drawMap();
       clear();
       for(j = 0; j < MAPSIZE; j++){
         map[j] -= speed;
        }
       if(map[ObstacleIndex] < 40){
         ObstacleIndex++;
       }
   }

   speed = 2;
   while(1){
      drawDinosaur(GROUNDBOUND);
       drawMap();
       clear();
       for(j = 0; j < MAPSIZE; j++){
         map[j] -= speed;
        }
       if(map[ObstacleIndex] < 40){
         ObstacleIndex++;
         break;
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
      DpointY = y + 30;
      for(j = 0; j < MAPSIZE; j++){
       map[j] -= speed;
      }
      if(map[ObstacleIndex] < 40)
        ObstacleIndex++;
      OpointX = map[ObstacleIndex];
     checkCrash();

      if(y <= UPBOUND) break;
    }

     while(1){
      drawDinosaur(y);
      drawMap();
      clear();
      y += 3;
      DpointY = y + 30;
      for(j = 0; j < MAPSIZE; j++){
       map[j] -= speed;
      }
      if(map[ObstacleIndex] < 40)
         ObstacleIndex++;
      OpointX = map[ObstacleIndex];
      checkCrash();

      if(y >= GROUNDBOUND) break;
     }
}



///////////////itoa function//////////////////////////////////////////////////////////////////////////////////////
void itoa(int num, char *pStr){
    int radix = 10;
    int deg = 1;
    int i, cnt = 0;

    while(1){
        if((num / deg) > 0)
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
   GrContextForegroundSet(&sContext, ClrBlack);
   for (i0 = 0; i0<20; i0++) //for LCD Background Reset
      GrLineDraw(&sContext, 280, 50 + i0, 330, 50 + i0);
   GrContextFontSet(&sContext, g_psFontCm12);
   GrContextForegroundSet(&sContext, ClrSilver);
}
////////////////////////////////////////////////////////////////////////////////////////////////
//    if(pStr == NULL) return ;
//
//    if(num < 0){
//        *pStr = '-';
//        num *= -1;
//        pStr++;
//    }
////////////////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
   int k;
   uint32_t ui32SysClock;
   uint32_t ulIdy;

   ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                 SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                 SYSCTL_CFG_VCO_480), 120000000);

//Button
   //Enable Port N & Port Q & Port P & Port E
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); // RED LED도 포함
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ); // GPIOQ 클럭 공급, PQ7 - GREEN

      //configure PP1(select button), PN3(Up button), PE5(Down button) as GPIO input
      GPIOPinTypeGPIOInput(GPIO_PORTP_BASE, GPIO_PIN_1);
      GPIOPinTypeGPIOInput(GPIO_PORTN_BASE, GPIO_PIN_3);
      GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5);

      // LED
      GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_5); // RED, polling 모드시 RED LED ON
   GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, GPIO_PIN_7); // GREEN, interrupt 모드시 GREEN LED ON

      //configure the event detected for input port
   GPIOIntTypeSet(GPIO_PORTP_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE);
   GPIOIntTypeSet(GPIO_PORTN_BASE, GPIO_PIN_3, GPIO_FALLING_EDGE);
   GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_FALLING_EDGE);

   //enable interrupt in GPIO
   GPIOIntEnable(GPIO_PORTP_BASE, GPIO_INT_PIN_1);
   GPIOIntEnable(GPIO_PORTN_BASE, GPIO_INT_PIN_3);
   GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_5);

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


//Touch Screen
   TouchScreenInit(ui32SysClock);
   TouchScreenCallbackSet(TSHandler);





//   uint32_t up = GPIOIntStatus(GPIO_PORTN_BASE, true); //up button
//               GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_3);
//               if((up & 0x01<<3)!=0)
//   uint32_t down = GPIOIntStatus(GPIO_PORTE_BASE, true); //down button
//         GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_5);
//         if((down & 0x01<<5)!=0)


//Start Frame.
   GrContextForegroundSet(&sContext, ClrSilver);
   GrContextFontSet(&sContext, g_psFontCm20);
   GrStringDrawCentered(&sContext, "Select Dinosaur!", -1,
               GrContextDpyWidthGet(&sContext) / 2,
               GrContextDpyHeightGet(&sContext) / 8, 0);

   //( 0 : GOld, 1 : Red, 2 : DeepPink, 3 : ClrGainsboro)
      int arrowY = 60;
      color = 0;
   drawDinosaur(60);
   color = 1;
   drawDinosaur(100);
   color = 2;
   drawDinosaur(140);
   color = 3;
   drawDinosaur(180);
   GrContextForegroundSet(&sContext, ClrSilver);
   drawArrow(100, arrowY);


   while(1){


      uint32_t select = GPIOIntStatus(GPIO_PORTP_BASE, true); //select button
               GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_1);
      if((select & 0x01<<1)!=0){
         if(arrowY == 60)
            color = 0;
         else if(arrowY == 100)
            color = 1;
         else if(arrowY == 140)
            color = 2;
         else if(arrowY == 180)
            color = 3;
         break;
      }

      uint32_t up = GPIOIntStatus(GPIO_PORTN_BASE, true); //up button
      GPIOIntClear(GPIO_PORTN_BASE, GPIO_INT_PIN_3);
      if((up & 0x01<<3)!=0) {//(arrowY==60)?arrowY=180:arrowY+=40;
         deleteArrow(100, arrowY);
         arrowY-=40;
         if(arrowY<60)
            arrowY=180;
         drawArrow(100, arrowY);
      }

      uint32_t down = GPIOIntStatus(GPIO_PORTE_BASE, true); //down button
      GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_5);
      if((down & 0x01<<5)!=0) {//(arrowY==180)?arrowY=60:arrowY-=40;
         deleteArrow(100, arrowY);
         arrowY+=40;
         if(arrowY>180)
            arrowY=60;
         drawArrow(100, arrowY);
      }
   }
   int i;
   GrFlush(&sContext);
   GrContextForegroundSet(&sContext, ClrBlack);
   for(i = 0; i < 240; i++)
      GrLineDraw(&sContext, 0, i, 330, i);

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
    GrStringDraw(&sContext, "Record : ", -1, 155, 55, 0);
    GrContextForegroundSet(&sContext, ClrGold);
    GrStringDraw(&sContext, "280", -1, 210, 55, 0);

    GrContextForegroundSet(&sContext, ClrRed);
    GrContextFontSet(&sContext, g_psFontCm12);
    GrStringDraw(&sContext, "Score : ", -1, 240, 55, 0);

    GrContextForegroundSet(&sContext, ClrBlue);
    GrContextFontSet(&sContext, g_psFontCm12);
    GrStringDrawCentered(&sContext, "===Progress===", -1,
                GrContextDpyWidthGet(&sContext) / 2,
                GrContextDpyHeightGet(&sContext) *9 / 10, 0);
    ChangeMap(); ////////////////////////////////////////////////////////
    drawDinosaur(GROUNDBOUND); //////////////////////////////////////////
    drawMap(); //////////////////////////////////////////////////////////

    GrContextForegroundSet(&sContext, ClrGold);
    GrContextFontSet(&sContext, g_psFontCm20);
    GrStringDraw(&sContext, "Start to Press Screen", -1, 70, 100, 0);
    while(1) { if(touched) break; }

    GrContextForegroundSet(&sContext, ClrBlack);
    for(k=0; k<20; k++)
       GrLineDraw(&sContext, 60, 100+k, 300, 100+k);

    GrContextForegroundSet(&sContext, ClrGold);
    GrContextFontSet(&sContext, g_psFontCm20);
    GrStringDraw(&sContext, "3", -1, 150, 100, 0);
    SysCtlDelay(40000000);

    GrContextForegroundSet(&sContext, ClrBlack);
    for(k=0; k<20; k++)
       GrLineDraw(&sContext, 130, 100+k, 170, 100+k);
    GrContextForegroundSet(&sContext, ClrGold);
    GrStringDraw(&sContext, "2", -1, 150, 100, 0);
    SysCtlDelay(40000000);

    GrContextForegroundSet(&sContext, ClrBlack);
    for(k=0; k<20; k++)
       GrLineDraw(&sContext, 130, 100+k, 170, 100+k);
    GrContextForegroundSet(&sContext, ClrGold);
    GrStringDraw(&sContext, "1", -1, 150, 100, 0);
    SysCtlDelay(40000000);

    GrContextForegroundSet(&sContext, ClrBlack);
    for(k=0; k<20; k++)
       GrLineDraw(&sContext, 130, 100+k, 170, 100+k);
//Map
//Flush any cached drawing operation
     GrFlush(&sContext);
     TimerEnable(TIMER0_BASE, TIMER_A);

     OpointX = map[ObstacleIndex];
     while(1)
     {

      touched = false;
      while(!touched) // touch 하기 전 공룡은 가만히 있고 장애물만 왼쪽으로 계속 이동
      {
         GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_7, 0x00);   // Green OFF
           drawDinosaur(GROUNDBOUND);
          drawMap();
          clear();
          for(j = 0; j < MAPSIZE; j++){
            map[j] -= speed;
           }
          if(map[ObstacleIndex] < 40){
            ObstacleIndex++;
          }

          OpointX = map[ObstacleIndex];
         checkCrash();
      }

      if(state)
      {

         GPIOPinWrite(GPIO_PORTQ_BASE, GPIO_PIN_7, 0x01 << 7);   // Green On

         jump();
       state=0;
       touched = false;
      }
    }
}

void Timer0AIntHandler(void)
{
   TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
   itoa(++timenum, timer0);
   GrStringDraw(&sContext, timer0, -1, 285, 55, 0);
}
