/****************************************************************************
 Module
   LCDService.c

 Revision
   1.0.1

 Description
   This is the first service for the Test Harness under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/26/16 09:58 jec      filled in missing code for Lab 3
 10/19/16 13:24 jec      added comments about where to add deferal and recall
 01/12/15 21:47 jec      converted to LCD module for lab 3
 11/02/13 17:21 jec      added exercise of the event deferral/recall module
 08/05/13 20:33 jec      converted to test harness service
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "FourierService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

#include "LCD_Write.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "kiss_fft.h"

#ifndef M_PI
#define M_PI 3.14159265358979324
#endif

#define N 16

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)

// Define helpers
#define clrScrn() printf("\x1b[2J")
#define goHome()	printf("\x1b[1,1H")
#define clrLine()	printf("\x1b[K")
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static uint8_t DeferralQueueLength = 4;

// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event DeferralQueue[3+1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitLCDService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitFourierService( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  // Initialize the LCD
	// puts("Intializing the LCD\r\n");

  // Set up the Deferral Queue
  ES_InitDeferralQueueWith(DeferralQueue,DeferralQueueLength);

	//Put us into the initial pseudo-state to set up for the initial transition
	// CurrentState = InitPState;

	// Set up the short timer for inter-command timings
  ES_ShortTimerInit(MyPriority, SHORT_TIMER_UNUSED );

	printf("************************************\r\n");
	printf("********** HERE WE GO **************\r\n");
	printf("************************************\r\n");	
	
	kiss_fft_cpx in[N], out[N];
  size_t i;

  //for (i = 0; i < N; i++)
  //  in[i].r = in[i].i = 0;
  //TestFft("Zeroes (complex)", in, out);

  //for (i = 0; i < N; i++)
  //  in[i].r = 1, in[i].i = 0;
  //TestFft("Ones (complex)", in, out);

  for (i = 0; i < N; i++)
		in[i].r = sin(2 * M_PI * 4 * i / N), in[i].i = 0;
  TestFft("SineWave (complex)", in, out);
	
	printf("*********************************\r\n");
	printf("******* AND WE'RE DONE **********\r\n");
	printf("*********************************\r\n");	
	
	
  // Post the initial transition event
	// puts("Posting transition event\r\n");
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
    return true;
  } else {
     return false;
  }
}

/****************************************************************************
 Function
     PostLCDService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostFourierService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLCDService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event RunFourierService( ES_Event ThisEvent )
{
	//puts("LCD got message\r\n");
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

  return ReturnEvent;
}










void TestFft(const char* title, const kiss_fft_cpx in[N], kiss_fft_cpx out[N])
{
  kiss_fft_cfg cfg;

	puts("WE ARE GOING IN");
  printf("%s\n", title);

	cfg = kiss_fft_alloc(N, 0/*is_inverse_fft*/, NULL, NULL);
  if (cfg != NULL)
  {
    size_t i;

    kiss_fft(cfg, in, out);
    
    for (i = 0; i < N; i++)
      printf(" in[%2zu] = %+f , %+f    "
             "out[%2zu] = %+f , %+f\r\n",
             i, in[i].r, in[i].i,
             i, out[i].r, out[i].i);
  }
  else
  {
    printf("not enough memory?\n");
    exit(-1);
  }
}






















