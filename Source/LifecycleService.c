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

#define INACTIVITY_LENGTH 30
#define PASSAGE_OF_TIME_LENGTH 45

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
	ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER,TWO_SEC);

	// Initialize our inactivity timer
	ES_Timer_InitTimer(INACTIVITY_TIMER,ONE_SEC);
	
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
				// All the hardware is initialized. We can move to the welcome state
				printf("Lifecycle: Asking all services to perform Welcome Performance\r\n");
				PostingEvent.EventType = LIFECYCLE_START_WELCOME_PERFORMANCE;
				PostWatertubeService(PostingEvent);
				PostKnobService(PostingEvent);
				PostLEDService(PostingEvent);
				CurrentState = WelcomeState;
			} else {
				// Catch unhandled events
				printf("Lifecycle: In Psuedo Init State\r\n");
				printf("Lifecycle: All hardware has been initialized\r\n");
				printf("Lifecycle: Press 'h' to enter the Welcome State (Fire hardware initilized)\r\n\r\n");
			}
		break;
		
		case WelcomeState:
			if (ThisEvent.EventType==LIFECYCLE_WELCOME_COMPLETE){
				printf("\r\nLifecycle: Moving to Waiting State\r\n");
				printf("Lifecycle: Starting the passage of time timer\r\n");
				printf("Lifecycle: Starting the inactivity timer\r\n");
				ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER,ONE_SEC);
				ES_Timer_InitTimer(INACTIVITY_TIMER,ONE_SEC);
				CurrentState = WaitingState;
			} else {
				// Catch other events
				printf("Lifecycle: In Welcome State\r\n");
				printf("Lifecycle: Press 'j' to move to WAITING\r\n");
			}
		break;
	
		// The waiting state occurs when all sensors are sampling
		// This state is left when the passage of time is complete or there is inactivity
		case WaitingState:
			if (ThisEvent.EventType==ES_TIMEOUT){
				PassageOfTime += 1;
				printf("Lifecycle [waiting]: Passage of time at %i seconds\r\n",PassageOfTime);
				printf("Lifecycle [waiting]: Inactivity time at %i seconds\r\n",PassageOfTime);
			
				printf("Lifecycle [waiting]: Press '1-7' or 'q-u' to trigger servos\r\n");
				printf("Lifecycle [waiting]: Press 'i-8' to trigger knob vibrator\r\n");
				printf("Lifecycle [waiting]: Press 'm' to turn on the microphone service\r\n");
				printf("Lifecycle [waiting]: Press 'c' to fire the welcome performance\r\n");
				printf("Lifecycle [waiting]: Press 'x' to trigger finale (passage_of_time_complete)\r\n");
				printf("Lifecycle [waiting]: Press 'z' to trigger reset due to inactivity\r\n\r\n");
				ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER,TWO_SEC); // Reset the timer
			}
			else if (ThisEvent.EventType==LIFECYCLE_PASSAGE_OF_TIME_COMPLETE){
				// Some service told us to move to the finale
				printf("Lifecycle: Moving to Finale State");
				CurrentState = FinaleState;
				PostLifecycleService(ThisEvent);
			}
			else if (ThisEvent.EventType==LIFECYCLE_INACTIVITY_RESET){
				// Some service told us that we have been inactive too long
				printf("Lifecycle: Moving to Finale State\r\n");
				CurrentState = FinaleState;
				PostLifecycleService(ThisEvent);
			}
			else if (PassageOfTime>PASSAGE_OF_TIME_LENGTH){
				// Fire a passage of time on myself
				PostingEvent.EventType = LIFECYCLE_PASSAGE_OF_TIME_COMPLETE;
				PostLifecycleService(PostingEvent);
			}
			else if (PassageOfTime>INACTIVITY_LENGTH){
				// Fire an inactivity on myself
				PostingEvent.EventType = LIFECYCLE_PASSAGE_OF_TIME_COMPLETE;
				PostLifecycleService(PostingEvent);
			}
			else {
				// Catch unhandled events
				printf("\r\nLifecycle: WaitingState\r\n");
				printf("All sensors are sampling\r\n");
			}
		break;
	
		case FinaleState:
			if (ThisEvent.EventType==LIFECYCLE_RESET_ALL){
				// The user asked for a reset
				printf("\r\nLifecycle: STATE RESET\r\n");
				printf("Lifecycle: STATE RESET\r\n");
				printf("Lifecycle: Moving to welcome State\r\n");
				CurrentState = WelcomeState;
				PostLifecycleService(ThisEvent);
			} else  {
				// Catch other events
				printf("Lifecycle: In FinaleState\r\n");
				printf("Press 'v' to reset (everything)\r\n");
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
	// Microphone events
	if (ThisEvent.EventParam=='m'){
		printf("MICROPHONE_START\r\n");
		PsuedoEvent.EventType = MICROPHONE_START;
		PostMicrophoneService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='n'){
		printf("MICROPHONE_STOP\r\n");
		PsuedoEvent.EventType = MICROPHONE_STOP;
		PostMicrophoneService(PsuedoEvent);
	}
	// Tube empty events (keys 1-7)
	if (ThisEvent.EventParam=='1'){
		printf("CHANGE_WATER_1 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_1;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='2'){
		printf("CHANGE_WATER_2 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_2;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
		if (ThisEvent.EventParam=='3'){
		printf("CHANGE_WATER_3 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_3;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='4'){
		printf("CHANGE_WATER_4 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_4;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='5'){
		printf("CHANGE_WATER_5 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_5;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='6'){
		printf("CHANGE_WATER_6 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_6;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='7'){
		printf("CHANGE_WATER_7 with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_7;
		PsuedoEvent.EventParam = 0;	
		PostWatertubeService(PsuedoEvent);
	}
	// Knob service
		if (ThisEvent.EventParam=='8'){
		printf("CHANGE_KNOB_VIBRATION with value 0\r\n");
		PsuedoEvent.EventType = CHANGE_KNOB_VIBRATION;
		PsuedoEvent.EventParam = 0;	
		PostKnobService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='i'){
		printf("CHANGE_KNOB_VIBRATION with value 3300\r\n");
		PsuedoEvent.EventType = CHANGE_KNOB_VIBRATION;
		PsuedoEvent.EventParam = 3300;	
		PostKnobService(PsuedoEvent);
	}
	// Tube fill events (q-u)
		if (ThisEvent.EventParam=='q'){
		printf("CHANGE_WATER_1 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_1;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='w'){
		printf("CHANGE_WATER_2 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_2;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='e'){
		printf("CHANGE_WATER_3 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_3;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='r'){
		printf("CHANGE_WATER_4 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_4;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='t'){
		printf("CHANGE_WATER_5 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_5;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='y'){
		printf("CHANGE_WATER_6 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_6;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='u'){
		printf("CHANGE_WATER_7 with value 4096\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_7;
		PsuedoEvent.EventParam = 4096;	
		PostWatertubeService(PsuedoEvent);
	}
	// Lifecycle Events 
	if (ThisEvent.EventParam==' '){
		// When all the services have been initialized
		printf("NO_EVENT\r\n"); 
		PsuedoEvent.EventType = ES_NO_EVENT;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='h'){
		// When all the services have been initialized
		printf("LIFECYCLE_HARDWARE_INITALIZED\r\n"); 
		PsuedoEvent.EventType = LIFECYCLE_HARDWARE_INITALIZED;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='j'){
		// When the user walks away
		printf("LIFECYCLE_WELCOME_COMPLETE\r\n");
		PsuedoEvent.EventType = LIFECYCLE_WELCOME_COMPLETE;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='x'){\
		// When the user is out of time
		printf("LIFECYCLE_PASSAGE_OF_TIME_COMPLETE\r\n");
		PsuedoEvent.EventType = LIFECYCLE_PASSAGE_OF_TIME_COMPLETE;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='z'){
		// When the user walks away
		printf("LIFECYCLE_INACTIVITY_RESET\r\n");
		PsuedoEvent.EventType = LIFECYCLE_INACTIVITY_RESET;
		PostLifecycleService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='v'){
		// When the perfomance is done and the user presses reset
		printf("LIFECYCLE_RESET_ALL\r\n");
		printf("Lifecycle: Resettting ALL services\r\n");
		PsuedoEvent.EventType = LIFECYCLE_RESET_ALL;
		ES_PostAll(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='c'){
		// When the perfomance is done and the user presses reset
		printf("LIFECYCLE_START_WELCOME_PERFORMANCE\r\n");
		printf("Lifecycle: Firing welcome event on all \r\n");
		PsuedoEvent.EventType = LIFECYCLE_START_WELCOME_PERFORMANCE;
		ES_PostAll(PsuedoEvent);
	}
}
