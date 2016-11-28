
/****************************************************************************
 Module
   knobService.c

 Revision
   1.0.1

 Description
   This is a simple service controlling the vibrating knob
	 The knob takes samples from the ADC at 100 millisecond intervals
	 If a change is detected, the knob starts vibrating for 1 second
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
#include "ADMulti.h" // ADC Library
#include "PWM10Tiva.h"
#include "ResetService.h"
#include "KnobService.h"

/*----------------------------- Module Defines ----------------------------*/
#define VIBRATION_TIME 1000

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static uint32_t getADCStateKnob(void);
static uint32_t getADCDiffKnob(uint32_t CurrentADCStateKnob, uint32_t LastADCStateKnob);
static void startKnobVibration(uint32_t Voltage);
static void stopKnobVibration(void);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static KnobState_t CurrentState;
static uint32_t LastADCStateKnob;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitKnobService

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority
		 Initialize the ADC
		 Initialize the PWM
****************************************************************************/
bool InitKnobService ( uint8_t Priority )
{
  
	ES_Event ThisEvent;

  MyPriority = Priority;
	
	printf("Initializing Knob Service\n\r");
	
	// Initialize the Tiva for 10 PWM ports
	// Multiple services do this, but they all intialize 10 ports
	PWM_TIVA_Init(10);
  
  ThisEvent.EventType = ES_INIT;
	LastADCStateKnob = getADCStateKnob();
	
	// The knon does not vibrate on startup
	CurrentState = KnobVibrating;
	
	// Intialize the KNOB_VIBRATION_TIMER
	ES_Timer_Init(ES_Timer_RATE_1mS);
	
	// The Knob Serices uses port 8 on the Tiva ADC
	// Initializing group four, initilizes port 7&8
	PWM_TIVA_SetPeriod(1250, 4);

  if (ES_PostToService( MyPriority, ThisEvent) == true) {
      return true;
  } else {
      return false;
  }
}

/****************************************************************************
 Function
     PostTemplateService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostKnobService( ES_Event ThisEvent )
{
	printf("Knob Service: Message posted Knob service\n\r");
  return ES_PostToService( MyPriority, ThisEvent);
}


bool CheckKnobEvents(void) {
	bool ReturnVal = false;
	ES_Event TouchEvent;
	
	uint32_t CurrentADCStateKnob = getADCStateKnob();
	uint32_t diff = getADCDiffKnob(CurrentADCStateKnob, LastADCStateKnob);
	
	if (diff >=300){
		// Post to my own queue
		TouchEvent.EventType=CHANGE_KNOB_VIBRATION;
		TouchEvent.EventParam = CurrentADCStateKnob;
		PostKnobService(TouchEvent);
		// Tell the reset service that there was interaction
		TouchEvent.EventType = ES_INTERACTION;
		PostResetService(TouchEvent);
		// Prepare for next check
		LastADCStateKnob = CurrentADCStateKnob;
		ReturnVal = true;
	}
	return ReturnVal;
}



/****************************************************************************
 Function
    RunKnobService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunKnobService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
	
	switch (CurrentState){
		// In this state the knob is vibrating
		// Exit this state when the timer stops
		case KnobVibrating:
			if(ThisEvent.EventType == CHANGE_KNOB_VIBRATION){
				startKnobVibration(ThisEvent.EventParam);
				ES_Timer_StopTimer(KNOB_VIBRATION_TIMER);
				ES_Timer_InitTimer(KNOB_VIBRATION_TIMER, VIBRATION_TIME);
			}
			if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == KNOB_VIBRATION_TIMER){
				// Move to the sleeping mode
				CurrentState = KnobSleeping;
				stopKnobVibration();
			}
		break;
	
		// In this state the knob is stopped
		// Exit this state when the knob is moved
		case KnobSleeping:
			if(ThisEvent.EventType == CHANGE_KNOB_VIBRATION){
				// Wake up and start vibrating
				startKnobVibration(ThisEvent.EventParam);
				ES_Timer_InitTimer(KNOB_VIBRATION_TIMER, VIBRATION_TIME);
				CurrentState = KnobVibrating;
			}
		break;
	}


	//PWM_TIVA_SetFreq( uint16_t reqFreq, uint8_t group);

  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/


/****************************************************************************
 Function
    startKnobVibration

 Parameters
   None

 Returns
   Nothing

 Description
    Start the knob vibrating 
****************************************************************************/
void startKnobVibration(uint32_t Voltage) {
	printf("KnobService: Vibrating at %i \r\n",Voltage);
	uint16_t PulseWidth = (Voltage*1250/4096);
	if (PulseWidth>1250){
		printf("WARNING: Vibrator pulse width=%i",PulseWidth);
	} else {
		PWM_TIVA_SetPulseWidth(PulseWidth,8);
	}
}



/****************************************************************************
 Function
    stopKnobVibration

 Parameters
   None

 Returns
   Nothing

 Description
    Stop the knob vibrating 
****************************************************************************/
void stopKnobVibration(void) {
		uint8_t stopped = 0;
		printf("KnobService: Sleeping\r\n");
		PWM_TIVA_SetPulseWidth(stopped,8);
}




/****************************************************************************
 Function
    getADCStateKnob

 Parameters
   void

 Returns
   uint32_t Voltage. Return the current voltage at the ADC knob

****************************************************************************/
uint32_t getADCStateKnob(void) {
	uint32_t ADInput[4];
	uint32_t CurrentInputKnob;
	ADC_MultiRead(ADInput);
	CurrentInputKnob = ADInput[2]; // Get ADC data from PE0
	//printf("Current Knob Input is %u.\r\n", CurrentInputKnob);
	return CurrentInputKnob;
}



/****************************************************************************
 Function
    getADCDiffKnob

 Parameters
   CurrentADCState, LastADCState

 Returns
   uint32_t Voltage. Return the difference between the previous ADC state and the current one

****************************************************************************/
uint32_t getADCDiffKnob(uint32_t CurrentADCStateKnob, uint32_t LastADCStateKnob) {
	if (CurrentADCStateKnob >= LastADCStateKnob) {
		return (CurrentADCStateKnob - LastADCStateKnob);
	} else {
		return (LastADCStateKnob - CurrentADCStateKnob);
	}
}


