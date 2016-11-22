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
static uint32_t getADCState(void);
static uint32_t getADCDiff(uint32_t CurrentADCState, uint32_t LastADCState);
static uint32_t getADCStateKnob(void);
static uint32_t getADCDiffKnob(uint32_t CurrentADCStateKnob, uint32_t LastADCStateKnob);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static bool LastButtonState;
static uint16_t InteractionTime;
static uint32_t LastADCState;
static uint32_t LastADCStateKnob;
static ResetState_t CurrentState;

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
	//Initialize the MyPriority variable with the passed in parameter.
	MyPriority = Priority;
	//Initialize the port line to monitor the button
	HWREG(SYSCTL_RCGCGPIO)|= SYSCTL_RCGCGPIO_R1; 
  while ((HWREG(SYSCTL_RCGCGPIO) & SYSCTL_RCGCGPIO_R1) != SYSCTL_RCGCGPIO_R1);
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN)|= GPIO_PIN_3;
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR)&= ~GPIO_PIN_3;
	
	InteractionTime = 0;
	//Initialize all the last states
	LastButtonState = getButtonState();
	LastADCState = getADCState();
	LastADCStateKnob = getADCStateKnob();
	
	CurrentState = Running;
	
	//Set up timer system
	ES_Timer_Init(ES_Timer_RATE_1mS);
	ES_Timer_InitTimer(INACTIVITY_TIMER, ONE_MINUTE);
	printf("Reset Service Initializated.\r\n");
	return true;
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
			ThisEvent.EventType = ES_RESET;
			PostLEDService(ThisEvent);
			PostWatertubeService(ThisEvent);
			PostMicrophoneService(ThisEvent);
		}
	}

	LastButtonState = CurrentButtonState;
	return ReturnVal;
}

/****************************************************************************
 Function
     CheckInteraction

 Parameters
     None

 Returns
     bool true if an event posted
****************************************************************************/
/*bool CheckInteraction(void){
	bool ReturnVal = false;
	bool CurrentADCState = getADCState();
	bool CurrentADCStateKnob = getADCStateKnob();
	uint32_t diffADCLED = getADCDiff(CurrentADCState, LastADCState);
	uint32_t diffADCKnob = getADCDiffKnob(CurrentADCStateKnob, LastADCStateKnob);
	ES_Event ThisEvent;

	// If user either touches the resistor strip or twists the knob
	if ((diffADCLED>=100) || (diffADCKnob>=100)) {
		ReturnVal = true;
		ThisEvent.EventType = ES_INTERACTION;
		PostResetService(ThisEvent);
	}
	
	LastADCState = CurrentADCState;
	LastADCStateKnob = CurrentADCStateKnob;
	return ReturnVal;
}*/

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

	if (CurrentState == Running) {
		if (ThisEvent.EventType == ES_WELCOME_COMPLETE) {		
			printf("Reset: Welcome back\r\n");
			ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER, HALF_SEC); //#define PASSAGE_OF_TIME_TIMER 2
			ES_Timer_StopTimer(INACTIVITY_TIMER);
			ES_Timer_InitTimer(INACTIVITY_TIMER, THIRTY_SEC); //#define INACTIVITY_TIMER 3
			InteractionTime = 0;
		} else if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PASSAGE_OF_TIME_TIMER && InteractionTime < 90) {
			InteractionTime++;
			printf("Reset: Interaction Time %i\r\n",InteractionTime);
			PostEvent.EventType = CHANGE_WATER_7;
			PostEvent.EventParam = (InteractionTime/90.0*4096);
			PostWatertubeService(PostEvent);
			ES_Timer_InitTimer(PASSAGE_OF_TIME_TIMER, HALF_SEC);
			// Print keyboard instructions
			printf("Reset [waiting]: Press '1-7' or 'q-u' to trigger servos\r\n");
			printf("Reset [waiting]: Press 'i-8' to trigger knob vibrator\r\n");
			printf("Reset [waiting]: Press 'm' to turn on the microphone service\r\n");
			printf("Reset [waiting]: Press 'c' to fire the welcome performance\r\n");
			printf("Reset [waiting]: Press 'x' to trigger finale (passage_of_time_complete)\r\n");
			printf("Reset [waiting]: Press 'z' to trigger reset due to inactivity\r\n\r\n");
		} else if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == PASSAGE_OF_TIME_TIMER) && (InteractionTime == 90)) { // Main timer expires
			printf("Reset: Passage of time complete\r\n");
			PostEvent.EventType = ES_RESET;
			PostLEDService(PostEvent);
			PostWatertubeService(PostEvent);
			PostMicrophoneService(PostEvent);
		} else if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == INACTIVITY_TIMER)) { // Inactivity timer expires
			PostEvent.EventType = ES_RESET;
			PostLEDService(PostEvent);
			PostWatertubeService(PostEvent);
			PostMicrophoneService(PostEvent);
			// Stop passage of time
			ES_Timer_StopTimer(PASSAGE_OF_TIME_TIMER);
			InteractionTime = 100;
			printf("Please STOP!!\r\n");
		} else if (ThisEvent.EventType == ES_INTERACTION) {
			ES_Timer_StopTimer(INACTIVITY_TIMER);
			ES_Timer_InitTimer(INACTIVITY_TIMER, THIRTY_SEC);
			PostEvent.EventType = ES_INTERACTION; // Pull all of the three services out of sleeping mode
			PostLEDService(PostEvent);
			PostWatertubeService(PostEvent);
			PostMicrophoneService(PostEvent);
			// Now we wait for ES_WELCOME_COMPLETE from LEDService
		}
	}
	return ReturnEvent; 
}

/***************************************************************************
 private functions
 ***************************************************************************/
bool getButtonState(void) {
	uint8_t ButtonState = HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) & BIT3HI;
	if (ButtonState) {
		return true;
	} else {
		return false;
	}
}

uint32_t getADCState(void) {
	uint32_t ADInput[4];
	uint32_t CurrentInput;
	ADC_MultiRead(ADInput);
	CurrentInput = ADInput[1]; // Get ADC data from PE1
	//printf("CurrentInput is %u.\r\n", CurrentInput);
	return CurrentInput;
}

uint32_t getADCDiff(uint32_t CurrentADCState, uint32_t LastADCState) {
	if (CurrentADCState >= LastADCState) {
		return (CurrentADCState - LastADCState);
	} else {
		return (LastADCState - CurrentADCState);
	}
}

uint32_t getADCStateKnob(void) {
	uint32_t ADInput[4];
	uint32_t CurrentInputKnob;
	ADC_MultiRead(ADInput);
	CurrentInputKnob = ADInput[2]; // Get ADC data from PE0
	//printf("Current Knob Input is %u.\r\n", CurrentInputKnob);
	return CurrentInputKnob;
}
uint32_t getADCDiffKnob(uint32_t CurrentADCStateKnob, uint32_t LastADCStateKnob) {
	if (CurrentADCStateKnob >= LastADCStateKnob) {
		return (CurrentADCStateKnob - LastADCStateKnob);
	} else {
		return (LastADCStateKnob - CurrentADCStateKnob);
	}
}



