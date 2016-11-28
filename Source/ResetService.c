/****************************************************************************
 Module
		ResetService.c

 Revision
 ResetService controls:
	1) Welcome performance
	2) Passage of time
	3) Final performance	
	it only post messages to the hardware services (Water tube and LED service)
    
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "ES_DeferRecall.h"
#include "ES_Timers.h"
#include "termio.h"

// the headers to access the GPIO subsystem
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// the headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "ADMulti.h" // ADC Library
#include "PWM10Tiva.h" // PWM Library

// The headers from other services of this project
#include "WatertubeService.h"
#include "MicrophoneService.h"
#include "KnobService.h"
#include "LEDService.h"
#include "ResetService.h"

#include "BITDEFS.H"

/*----------------------------- Module Defines ----------------------------*/
#define VERBOSE 0
#define ALL_BITS (0xff<<2)
// these times assume a 1.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC/2)
#define FORTY_FIVE_SEC (ONE_SEC*45)
#define THIRTY_SEC (ONE_SEC*30)
#define ONE_MINUTE (ONE_SEC*60)


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static bool getButtonState(void);
static void resetWake(void);
static void resetSleep(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint16_t InteractionTime;
static ResetState_t CurrentState;
static uint8_t LastButtonState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitializeResetService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
****************************************************************************/
bool InitializeResetService ( uint8_t Priority )
{
	ES_Event ThisEvent;
	
	//Initialize the MyPriority variable with the passed in parameter.
	MyPriority = Priority;
	
	//Initialize the port line to monitor the button
	HWREG(SYSCTL_RCGCGPIO)|= SYSCTL_RCGCGPIO_R1; 
  while ((HWREG(SYSCTL_RCGCGPIO) & SYSCTL_RCGCGPIO_R1) != SYSCTL_RCGCGPIO_R1);
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN)|= GPIO_PIN_3;
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR)&= ~GPIO_PIN_3;
	
	InteractionTime = 0;
	CurrentState = ResetInit;
	
	//Set up timer system
	ES_Timer_Init(ES_Timer_RATE_1mS);
	ES_Timer_InitTimer(INACTIVITY_TIMER, ONE_MINUTE);
	
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
     CheckButtonEvents

 Parameters
     None

 Returns
     bool true if an event posted
****************************************************************************/
bool CheckButtonEvents(void) {
	// Local ReturnVal = False, CurrentButtonState
	bool ReturnVal = false;
	bool CurrentButtonState;
	ES_Event ThisEvent;
	// Set CurrentButtonState to state read from port pin
	CurrentButtonState = getButtonState();
	
	if (CurrentButtonState != LastButtonState) {
	// If the CurrentButtonState is different from the LastButtonState
		ReturnVal = true;
		if (CurrentButtonState == true) {
			// If the reset button is down, then post reset event
			ThisEvent.EventType = ES_RESET_BUTTON;
			printf("RESET BUTTON PRESSED\r\n");
			PostResetService(ThisEvent);
		}
	}

	LastButtonState = CurrentButtonState;
	return ReturnVal;
}

/****************************************************************************
 Function
     PostResetService

 Parameters
     None

 Returns
     bool true if an event posted
****************************************************************************/
bool PostResetService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunResetService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunResetService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	ES_Event PostEvent;

	switch (CurrentState){
		// Psuedo Init State
		// Once-off intialization and immediate transition to welcome state
		// Future resets should jump straight to the welcome state
		// Exit immediately after setting timers ect
		case ResetInit:
			CurrentState = ResetWelcome;
			printf("ResetService: Initializated.\r\n");
		break;
			
		// Welcome State
		// The welcome performance is being performed
		// Exit when the LED welcome performance is complete
		case ResetWelcome:
			if (ThisEvent.EventType == ES_WELCOME_COMPLETE) {		
				printf("ResetService: Welcome Performance is Complete\r\n");
				ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER, HALF_SEC);
				ES_Timer_StopTimer(INACTIVITY_TIMER);
				ES_Timer_InitTimer(INACTIVITY_TIMER, THIRTY_SEC);
				// Start the microphone sampling
				PostEvent.EventType = MICROPHONE_START;
				PostMicrophoneService(PostEvent);
				// Move to the welcome state
				CurrentState = ResetRunning;
				InteractionTime = 0;
			} else if (ThisEvent.EventType == ES_RESET_BUTTON) {
				// Reset button pressed during welcome mode
				printf("ResetService [Welcome]: Resetting\r\n");
				resetSleep();
				resetWake();
				// Start the microphone sampling
				PostEvent.EventType = MICROPHONE_START;
				PostMicrophoneService(PostEvent);
				CurrentState = ResetWelcome;
			}
		break;
			
		// Running State
		// All services are waiting for feedback
		// Exit when the inactivity timer expires or 45 seconds is up
		case ResetRunning:
			if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PASSAGE_OF_TIME_TIMER && InteractionTime < 90) {
				// Normal running loop
				printf("Reset: Interaction Time %i\r\n",InteractionTime);
				InteractionTime++;
				// Increase the passage of time water tube
				PostEvent.EventType = CHANGE_WATER_7;
				PostEvent.EventParam = (InteractionTime/90.0*4096);
				PostWatertubeService(PostEvent);
				ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER, HALF_SEC);
				// Print keyboard instructions
				if (VERBOSE){
					printf("Reset [waiting]: Press '1-7' or 'q-u' to trigger servos\r\n");
					printf("Reset [waiting]: Press 'i-8' to trigger knob vibrator\r\n");
					printf("Reset [waiting]: Press 'm' to turn on the microphone service\r\n");
					printf("Reset [waiting]: Press 'c' to fire the welcome performance\r\n");
					printf("Reset [waiting]: Press 'x' to trigger reset (inactivity/timeout)\r\n");
				}
				
		  } else if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PASSAGE_OF_TIME_TIMER && InteractionTime >= 90) { 
				// Main timer expired. Notify all service and move to Sleeping
				PostEvent.EventType = ES_SLEEP;
				PostResetService(PostEvent);
			} else if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == INACTIVITY_TIMER) { 
				// Inactivity timer expired. 
				PostEvent.EventType = ES_SLEEP;
				PostResetService(PostEvent);
			} else if (ThisEvent.EventType == ES_SLEEP){
				// Someone told me to reset everything
				// Notify all services and move to Sleeping
				printf("Reset: Inactivity expiration\r\n");
				resetSleep();
				CurrentState = ResetSleeping;
			} else if (ThisEvent.EventType == ES_RESET_BUTTON) {
				// Reset button pressed
				resetSleep();
				resetWake();
				CurrentState = ResetWelcome;
			}
		break;
		
		// Sleeping State
		// All services are sleeping after the performance has completed
		// Exit when any services posts an ES_INTERACTION
		case ResetSleeping:
			if (ThisEvent.EventType == ES_INTERACTION || ThisEvent.EventType == ES_RESET_BUTTON) {
				printf("ResetService [Sleeping]: Resetting\r\n");
				// Wake all the services and reset timers
				resetWake();
				// Move to the welcome state
				CurrentState = ResetWelcome;
			}
		break;
	}
	return ReturnEvent;
}


/***************************************************************************
 private functions
 ***************************************************************************/

static void resetSleep(void) {
	// Put all the service to sleep
	ES_Event PostEvent;
	PostEvent.EventType = ES_SLEEP;
	PostLEDService(PostEvent);
	PostWatertubeService(PostEvent);
	PostMicrophoneService(PostEvent);
	CurrentState = ResetSleeping;
}


static void resetWake(void){
	// Pull all of the three services out of sleeping mode
	ES_Event PostEvent;
	PostEvent.EventType = ES_WAKE; 
	PostLEDService(PostEvent);
	PostWatertubeService(PostEvent);
	PostMicrophoneService(PostEvent);
	// Reset the inactivity timer
	ES_Timer_StopTimer(INACTIVITY_TIMER);
	ES_Timer_InitTimer(INACTIVITY_TIMER, THIRTY_SEC);
}


bool getButtonState(void) {
	uint8_t ButtonState = HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) & BIT3HI;
	if (ButtonState) {
		return true;
	} else {
		return false;
	}
}


