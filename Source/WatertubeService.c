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
#include "ES_Port.h"

#include "termio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

#include "ADMulti.h" // ADC Library
#include "PWM10Tiva.h" // PWM Library

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

/*---------------------------- Private Functions ---------------------------*/
void setWatertube(uint8_t tubeNumber,uint16_t waterHeight);

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
  ThisEvent.EventType = ES_INIT;

	printf("WatertubeService: Initializing 10 PWN ports\n\r");
	// Iniialize the PWN for 10 ports
	PWM_TIVA_Init(10);
	
	// Sets period of pwm signal to 20 ms (20000us/0.8us per tick = 25000)
	// We only need to set the firs three groups which gives us pins 0-6
	// The KnobServices uses pin 8 in group 4
	PWM_TIVA_SetPeriod(25000, 0);
	PWM_TIVA_SetPeriod(25000, 1);
	PWM_TIVA_SetPeriod(25000, 2);
	PWM_TIVA_SetPeriod(25000, 3);
	
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
			puts("Watertube: Entered the Intial State\r\n");
			// Immediately move to the Display/waiting state
			CurrentState = WaterDisplayState;
		break;

		case WaterDisplayState:			
			// Change the water height of tube 1
			if( ThisEvent.EventType == CHANGE_WATER_1){
					setWatertube(1,ThisEvent.EventParam);
			}
			// Change the water height of tube 2
			if( ThisEvent.EventType == CHANGE_WATER_2){
					setWatertube(2,ThisEvent.EventParam);
			}
			// Change the water height of tube 3
			if( ThisEvent.EventType == CHANGE_WATER_3){
					setWatertube(3,ThisEvent.EventParam);
			}
			// Change the water height of tube 4
			if( ThisEvent.EventType == CHANGE_WATER_4){
					setWatertube(4,ThisEvent.EventParam);
			}
			// Change the water height of tube 5
			if( ThisEvent.EventType == CHANGE_WATER_5){
					setWatertube(5,ThisEvent.EventParam);
			}
			// Change the water height of tube 6
			if( ThisEvent.EventType == CHANGE_WATER_6){
					setWatertube(6,ThisEvent.EventParam);
			}
			// Change the water height of tube 7
			if( ThisEvent.EventType == CHANGE_WATER_7){
					setWatertube(7,ThisEvent.EventParam);
			}
			
			// Reset all the tubes on sleep
			if( ThisEvent.EventType == ES_SLEEP){
				// Empty all the tubes
				uint16_t empty = 0;
				printf("Watertube: Emptying tubes.\r\n");
				setWatertube(1,empty);
				setWatertube(2,empty);
				setWatertube(3,empty);
				setWatertube(4,empty);
				setWatertube(5,empty);
				setWatertube(6,empty);
				setWatertube(7,empty);
				// Post my own sleeping event
				printf("Watertube: Sleeping\r\n");
				CurrentState = WaterSleepingState;
			}
		break;
	
		// The watertube sleeping mode
		case WaterSleepingState:
			if (ThisEvent.EventType == ES_WAKE) {
				printf("Watertube: Waking up.\r\n");
				CurrentState = WaterDisplayState;
			}
		break;
	}
  return ReturnEvent;
}


/****************************************************************************
 Function
    setWatertube

 Parameters
   uint8_t tubeNumber: The watertube number (1-7)
   uint16_t waterHeight: The water level (0-4096)

 Returns
   Nothing

 Description
   Adjust the water height
****************************************************************************/
void setWatertube(uint8_t tubeNumber, uint16_t waterHeight){
		uint8_t PWMNumber = tubeNumber-1;
		uint16_t PulseWidth = (waterHeight*2000/4096)+1000;
	
		if (tubeNumber<1 || tubeNumber>7){
			printf("Invalid water tube number %i\r\n",tubeNumber);
		} else if (waterHeight>4096){
			printf("Invalid water tube height %i\r\n",waterHeight);
		} else {
			//printf("Watertube: Change tube %i height to %i\n\r",tubeNumber,waterHeight);
			//printf("Watertube: Change pwm %i height to %i\n\r",PWMNumber,PulseWidth);
			PWM_TIVA_SetPulseWidth(PulseWidth,PWMNumber);
	}
}

