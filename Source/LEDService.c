/****************************************************************************
 Module
   LEDService.c

 Revision
   1.0.1

 Description
   Control the LED Strips

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

// Include my own Header
#include "LEDService.h"

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

static LEDState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitLEDService

Parameters: Intialize the Tiva Pots for ther LED

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
****************************************************************************/
bool InitLEDService( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;

	// TODO: Intialize the TIVA hardware for the LED
	
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
     PostLEDService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Put an event in the Queue for this service

****************************************************************************/
bool PostLEDService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLEDService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
	  Changes the color of the LED is response to CHANGE_LED_COLOR_X events
		Each LED can be controlled individually

****************************************************************************/
ES_Event RunLEDService( ES_Event ThisEvent )
{
	puts("LED got message\r\n");
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	
	if (CurrentState==LEDInitState){
		CurrentState = LEDMainState;
		return ReturnEvent;
	}

	switch (ThisEvent.EventType){
	
		case CHANGE_LED_1:
			// Change the color
		break;
		
		case CHANGE_LED_2:
			// Change the color	
		
		case CHANGE_LED_3:
			// Change the color
		break;
		
		case CHANGE_LED_4:
			// Change the color
		break;
		
		case CHANGE_LED_5:
			// Change the color
		break;
		
		case CHANGE_LED_6:
			// Change the color
		break;
		
		case CHANGE_LED_7:
			// Change the color
		break;
	
		case CHANGE_LED_8:
			// Chage the color
		break;
	}

  return ReturnEvent;
}
