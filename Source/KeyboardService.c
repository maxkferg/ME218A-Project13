/****************************************************************************
 Module
   KeyboardService.c

 Revision
   1.0.1

 Description
	KeyboardService controls the following:

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
#include "ResetService.h"
#include "KeyboardService.h"
#include "WatertubeService.h"
#include "MicrophoneService.h"
#include "KnobService.h"
#include "LEDService.h"


/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 1000
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
static KeyboardState_t CurrentState;



/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitKeyboardService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
****************************************************************************/
bool InitKeyboardService( uint8_t Priority )
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
     PostKeyboardService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
****************************************************************************/
bool PostKeyboardService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunKeyboardService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunKeyboardService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
  return ReturnEvent;
}




/****************************************************************************
 Function
     PostKeyboardEventGenerator

 Parameters
     EF_Event ThisEvent. A keypress event to convert to a ES_EVENT
 Returns
     void

 Description
     Posts an event to the relevant state machine
****************************************************************************/
void PostKeyboardEventGenerator( ES_Event ThisEvent ){
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
		printf("CHANGE_WATER_7 with value 2048\r\n");
		PsuedoEvent.EventType = CHANGE_WATER_7;
		PsuedoEvent.EventParam = 2048;	
		PostWatertubeService(PsuedoEvent);
	}
	// Lifecycle Events 
	if (ThisEvent.EventParam==' '){
		// When all the services have been initialized
		printf("NO_EVENT\r\n"); 
		PsuedoEvent.EventType = ES_NO_EVENT;
		PostResetService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='h'){
		// When all the services have been initialized
		printf("LIFECYCLE_HARDWARE_INITALIZED\r\n"); 
		PsuedoEvent.EventType = LIFECYCLE_HARDWARE_INITALIZED;
		PostResetService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='j'){
		// When the user walks away
		printf("LIFECYCLE_WELCOME_COMPLETE\r\n");
		PsuedoEvent.EventType = LIFECYCLE_WELCOME_COMPLETE;
		PostResetService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='x'){\
		// When the user is out of time
		printf("LIFECYCLE_PASSAGE_OF_TIME_COMPLETE\r\n");
		PsuedoEvent.EventType = LIFECYCLE_PASSAGE_OF_TIME_COMPLETE;
		PostResetService(PsuedoEvent);
	}
	if (ThisEvent.EventParam=='z'){
		// When the user walks away
		printf("LIFECYCLE_INACTIVITY_RESET\r\n");
		PsuedoEvent.EventType = LIFECYCLE_INACTIVITY_RESET;
		PostResetService(PsuedoEvent);
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
