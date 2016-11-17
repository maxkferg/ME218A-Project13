
/****************************************************************************
 Module
   TemplateService.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the 
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
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

#include "KnobService.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static uint32_t getADCStateKnob(void);
static uint32_t getADCDiffKnob(uint32_t CurrentADCStateKnob, uint32_t LastADCStateKnob);
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint32_t LastADCStateKnob;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes

****************************************************************************/
bool InitKnobService ( uint8_t Priority )
{
  
	ES_Event ThisEvent;

  MyPriority = Priority;
	
  //
	printf("Initializing Knob Service\n\r");
	
	// Initialize the Tiva for 10 PWM ports
	// Multiple services do this, but they all intialize 10 ports
	PWM_TIVA_Init(10);
  
  ThisEvent.EventType = ES_INIT;
	ES_Timer_InitTimer(KNOB_TIMER,1000);
	LastADCStateKnob = getADCStateKnob();
	printf("last adc state in init fcn = %u \r\n",LastADCStateKnob);
	
	// The Knob Serices uses port 8 on the Tiva ADC
	// Initializing group four, initilizes port 7&8
	PWM_TIVA_SetPeriod(1250, 4);

  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
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
	
	if (diff >=50){
		TouchEvent.EventType=CHANGE_KNOB_VIBRATION;
		TouchEvent.EventParam = CurrentADCStateKnob;
		PostKnobService(TouchEvent);
		printf("Event posted");
		LastADCStateKnob = CurrentADCStateKnob;
		ReturnVal = true;
	}
	return ReturnVal;
}



/****************************************************************************
 Function
    RunTemplateService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunKnobService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
	
	
//	ES_ShortTimerStart(TIMER_A, DelayTime);
  /********************************************
   in here you write your service code
   *******************************************/
	printf("in run Knob function\n\r");

	if(ThisEvent.EventType == CHANGE_KNOB_VIBRATION){//pwm channel 7
		uint32_t Voltage = ThisEvent.EventParam;
		printf("Got touch event CHANGE_VIBRATION %d\n\r",Voltage);
		int PulseWidth = (Voltage*1250/4096);
		printf("PulseWidth = %d",PulseWidth);
		PWM_TIVA_SetPulseWidth(PulseWidth,8);
	}
	
	//PWM_TIVA_SetFreq( uint16_t reqFreq, uint8_t group);

  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
uint32_t getADCStateKnob(void) {
	uint32_t ADInput[4];
	uint32_t CurrentInputKnob;
	ADC_MultiRead(ADInput);
	CurrentInputKnob = ADInput[0]; // Get ADC data from PE0
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


