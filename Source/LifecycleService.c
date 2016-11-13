/****************************************************************************
 Module
   LifeCycleService.c

 Revision
   1.0.1

 Description
	LifeCycleService controls the following:
	1) Welcome performance
	2) Passage of time
	3) Final performance	
	The Lifecycle service does not interface with any hardware
	it only post messages to the hardware services (Water tube and LED service)


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
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

// Header for this file
#include "LifecycleService.h"


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

// Keep track of the current state
static LifecycleState_t CurrentState;


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
****************************************************************************/
bool InitLifecycleService( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;

	CurrentState = InitPState;
	
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
     PostLifecycleService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
****************************************************************************/
bool PostLifecycleService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLifecycleService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunLifecycleService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

	switch (CurrentState){
		case InitPState:
			printf("Lifecycle: InitPState\r\n");
		break;

		case WelcomeState:
			printf("Lifecycle: WelcomeState\r\n");
		break;
	
		case MainState:
			printf("Lifecycle: MainState\r\n");
		break;
	
		case FinalState:
			printf("Lifecycle: FinalState\r\n");
		break;
	}

  return ReturnEvent;
}


