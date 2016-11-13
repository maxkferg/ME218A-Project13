/****************************************************************************
 Module
   SensitivityService.c

 Revision
   1.0.1

 Description
   Control the input on the sensitivity knob
	 Store the current sensitivity as a module variable
	 Post any sensitivy chnages to ALL services
	 Fire haptic feedback on the sensitivity knob

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

// Include my own header
#include "SensitivityService.h"

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

static SensitivityState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitSensitivityService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service

****************************************************************************/
bool InitSensitivityService( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
	
  // Initialize the Sensitivity Button
	puts("Intializing the Sensitivity Button\r\n");
	
	// Set the initla state of the sensitivity button
	CurrentState = SensitivityInitState;
	
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
     PostSensitivityService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostSensitivityService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}



/****************************************************************************
 Function
    RunSensitivityService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunSensitivityService( ES_Event ThisEvent )
{
	//puts("LCD got message\r\n");
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	
	switch (CurrentState){
		case SensitivityInitState:
			puts("SensitivityService: InitPState\r\n");
			puts("SensitivityService: Ready to accept sensitivity changes\r\n");
		  CurrentState = SensitivityMainState;
		break;

		case SensitivityMainState:
			puts("SensitivityService: Entered the SensitivityState\r\n");
			puts("SensitivityService: Entered the Welcome State\r\n");
		break;

		case SensitivityResetState:
			puts("SensitivityService: Entered the Reset State\r\n");
			CurrentState = SensitivityInitState;
		break;
	}

  return ReturnEvent;
}


