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

// Header for this file and others
#include "LifecycleService.h"
#include "WatertubeService.h"
#include "MicrophoneService.h"
#include "KnobService.h"
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

// Keep track of the current state
static LifecycleState_t CurrentState;

// Keep track of passage of time
static uint8_t PassageOfTime;


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
	
	// Initialize our passage of Time timer
	ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER,ONE_SEC);
	
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
	ES_Event PostingEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	PostingEvent.EventType = ES_NO_EVENT;

	switch (CurrentState){
		case InitPState:
			if (ThisEvent.EventType==LIFECYCLE_HARDWARE_INITALIZED){
				printf("Lifecycle: Hardeware Initialized. Moving all services to Welcome State\r\n");
				printf("Lifecycle: Asking all services to perform Welcome Performance\r\n");
				PostingEvent.EventType = LIFECYCLE_START_WELCOME_PERFORMANCE;
				PostWatertubeService(PostingEvent);
				PostKnobService(PostingEvent);
				PostLEDService(PostingEvent);
				CurrentState = WelcomeState;
			} else {
				printf("Lifecycle: In Psuedo Init State\r\n");
				printf("Lifecycle: All hardware has been initialized\r\n");
				printf("Lifecycle: Press H to enter the Welcome State (Fire hardware initilized)\r\n");
			}
		break;
		
		case WelcomeState:
			printf("Lifecycle: In Welcome State. Press j to skip to waiting state\r\n");
			if (ThisEvent.EventType==LIFECYCLE_WELCOME_COMPLETE){
				printf("Lifecycle: Moving to Waiting State\r\n");
				CurrentState = WaitingState;
			}
		break;
	
		case WaitingState:
			printf("Lifecycle: WaitingState\r\n");
		  printf("All sensors are sampling\r\n");
			if (ThisEvent.EventType==ES_TIMEOUT){
				PassageOfTime += 1;
				printf("Lifecycle: Passage of time at %i seconds\r\n",PassageOfTime);
				printf("Lifecycle: Press 't' to trigger finale (passage_of_time_complete)\r\n");
				printf("Lifecycle: Press 'i' to trigger reset due to inactivity\r\n");
			}
			if (ThisEvent.EventType==LIFECYCLE_PASSAGE_OF_TIME_COMPLETE){
				printf("Lifecycle: Moving to Finale State");
				CurrentState = FinaleState;
				PostLifecycleService(ThisEvent);
			}
			if (ThisEvent.EventType==LIFECYCLE_INACTIVITY_RESET){
				printf("Lifecycle: Moving to Finale State");
				CurrentState = FinaleState;
				PostLifecycleService(ThisEvent);
			}
		break;
	
		case FinaleState:
			printf("Lifecycle: FinalState\r\n");
			printf("Press 'r' to reset");
			if (ThisEvent.EventType==LIFECYCLE_RESET){
				printf("Lifecycle: Moving to welcome State");
				CurrentState = WelcomeState;
				PostLifecycleService(ThisEvent);
			}
		break;
	}

  return ReturnEvent;
}




/****************************************************************************
 Function
     PostLifecycleEventGenerator

 Parameters
     EF_Event ThisEvent. A keypress event to convert to a ES_EVENT
 Returns
     void

 Description
     Posts an event to the relevant state machine
****************************************************************************/
void PostLifecycleEventGenerator( ES_Event ThisEvent ){
	ES_Event PsuedoEvent;
	// Tube empty events (keys 1-7)
	if (ThisEvent.EventParam==1){
		printf("CHANGE_WATER_1 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_1;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam==2){
		printf("CHANGE_WATER_2 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_2;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
		if (ThisEvent.EventParam==3){
		printf("CHANGE_WATER_3 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_3;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam==4){
		printf("CHANGE_WATER_4 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_4;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam==5){
		printf("CHANGE_WATER_5 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_5;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam==6){
		printf("CHANGE_WATER_6 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_6;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam==7){
		printf("CHANGE_WATER_7 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_7;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	// Tube fill events (q-u)
		if (ThisEvent.EventParam=='q'){
		printf("CHANGE_WATER_1 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_1;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='w'){
		printf("CHANGE_WATER_2 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_2;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='e'){
		printf("CHANGE_WATER_3 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_3;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='r'){
		printf("CHANGE_WATER_4 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_4;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='t'){
		printf("CHANGE_WATER_5 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_5;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='y'){
		printf("CHANGE_WATER_6 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_6;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='u'){
		printf("CHANGE_WATER_7 with value 0");
		PsuedoEvent.EventType = CHANGE_WATER_7;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	// Lifecycle Events 
	if (ThisEvent.EventParam=='h'){
		// When all the services have been initialized
		printf("LIFECYCLE_HARDWARE_INITALIZED"); 
		PsuedoEvent.EventType = LIFECYCLE_HARDWARE_INITALIZED;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='t'){\
		// When the user is out of time
		printf("LIFECYCLE_PASSAGE_OF_TIME_COMPLETE");
		PsuedoEvent.EventType = LIFECYCLE_PASSAGE_OF_TIME_COMPLETE;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='i'){
		// When the user walks away
		printf("LIFECYCLE_INACTIVITY_RESET");
		PsuedoEvent.EventType = LIFECYCLE_INACTIVITY_RESET;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='r'){
		// When the perfomance is done and the user presses reset
		printf("LIFECYCLE_RESET");
		PsuedoEvent.EventType = LIFECYCLE_RESET;
		PostLifecycleService(PsuedoEvent);
	}
}

