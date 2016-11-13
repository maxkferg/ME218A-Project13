/****************************************************************************
 Module
   WaterTubeService.c

 Revision
   1.0.1

 Description
   This service controls the height of the water level
	 It is a hardware service, so it interfaces directly with the 
	 Tiva/PWM and servo motors. Note this module does not know anything
	 about the state of the performance. It just does what it is told.

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

// Include my own header
#include "WatertubeService.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)

// Define helpers
#define clrScrn() printf("\x1b[2J")
#define goHome()	printf("\x1b[1,1H")
#define clrLine()	printf("\x1b[K")


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static WatertubeState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitWatertubeService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
		
****************************************************************************/
bool InitWatertubeService( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  // Initialize the Water Tubes
	
	puts("Intializing the WaterTubes\r\n");

	CurrentState = WaterInitState;
	
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
     PostWatertubeService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

****************************************************************************/
bool PostWatertubeService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunWatertubeService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Wait for initialization to complete
	 Enter the WaterTubeState
   Allow the WaterTubes to be controlled by events
	 Wals wait for reset messages
****************************************************************************/
ES_Event RunWatertubeService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

	
	switch (CurrentState){
		case WaterInitState:
			puts("Watertube: Entered the WaterState\r\n");
			CurrentState = WaterDisplayState;
		break;

		case WaterDisplayState:
			puts("Watertube: Entered the WaterState\r\n");
			switch (ThisEvent.EventType){
				case CHANGE_WATER_1:
					// Change the height of water tube 1
				break;
				
				case CHANGE_WATER_2:
					// Change the height of water tube 2
				break;
				
				case CHANGE_WATER_3:
					// Change the height of water tube 3
				break;
				
				case CHANGE_WATER_4:
					// Change the height of water tube 4
				break;
				
				case CHANGE_WATER_5:
					// Change the height of water tube 5
				break;
				
				case CHANGE_WATER_6:
					// Change the height of water tube 6
				break;
				
				case CHANGE_WATER_7:
					// Change the height of water tube 7
				break;
				
				case CHANGE_WATER_8:
					// Change the height of water tube 8
				break;
			}
		break;
	
		case WaterResetState:
			puts("Watertube: Entered the Reset State\r\n");
			CurrentState = WaterDisplayState;
		break;
	}

	
  return ReturnEvent;
}
