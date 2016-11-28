/****************************************************************************
 Module
   ResistiveStripService.c

 Revision
   1.0.1

 Description
	 Monitor the resistive strip for changes
   If the voltage stays constant and non-zero
	 for a period of time, post a STRIP_CHANGED event to LEDService

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

// ADC Library
#include "ADMulti.h" 

// Include my own header
#include "LEDService.h"
#include "ResistiveStripService.h"
#include "ResetService.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define SAMPLING_INTERVAL 50 //100 ms

// Voltage constants on a 0-4095 scale 
#define MIN_VOLTAGE_THRESHOLD 150  // ADC must read at least this value to change mode
#define DIFF_VOLTAGE_THRESHOLD 200 // ADC must stay within +-this value to be constant
#define CONSTANT_DURATION_TICKS 8  // ADC must be be constant for this many ticks to change mode

/*---------------------------- Module Functions ---------------------------*/
static uint32_t getADCState(void);
static uint32_t getADCDiff(uint32_t CurrentADCState, uint32_t LastADCState);
static uint8_t getCurrentMode(uint16_t CurrentVoltage);
/*---------------------------- Module Variables ---------------------------*/

static uint8_t MyPriority;
static uint32_t LastADCState;

/*------------------------------ Module Code ------------------------------*/



/****************************************************************************
 Function
     InitResistiveStripService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service

****************************************************************************/
bool InitResistiveStripService( uint8_t Priority )
{
  MyPriority = Priority;
	
  // Initialize the Sensitivity Button
	puts("Resistive Strip: Intializing\r\n");
	
	//Set up timer system
	ES_Timer_InitTimer(RESISTIVE_STRIP_TIMER, SAMPLING_INTERVAL);
	
	// Sample ADC port line PE1 and use it to initialize the LastADCState variable
  LastADCState = getADCState();
	
	// There is no state machine so we do not need to post a transition event
	// Return true to mark that the intialization is complete
	puts("Resistive Strip: Initialization complete\r\n");
	return true;
}

/****************************************************************************
 Function
     PostSensitivityStripService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostResistiveStripService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}



/****************************************************************************
 Function
    RunSensitivityStripService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

****************************************************************************/
ES_Event RunResistiveStripService( ES_Event ThisEvent )
{
	ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	uint16_t CurrentMode;
	uint32_t CurrentADCState;
	uint32_t CurrentADCDiff;
	static uint8_t VoltageConstantTicks;
	
	if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == RESISTIVE_STRIP_TIMER){
		CurrentADCState = getADCState();
		CurrentADCDiff = getADCDiff(CurrentADCState,LastADCState);
		
		// Check if the ADC has changed this time period
		if (CurrentADCState>MIN_VOLTAGE_THRESHOLD && CurrentADCDiff<DIFF_VOLTAGE_THRESHOLD){
			VoltageConstantTicks +=1;
		} else {
			VoltageConstantTicks = 0;
		}
		
		// Check if the ADC has been constant for many ticks
		if (VoltageConstantTicks>CONSTANT_DURATION_TICKS){
			// Post the changed event and reset the constant resistance counter
			CurrentMode = getCurrentMode(CurrentADCState);
			ThisEvent.EventType = RESISTIVE_STRIP_CHANGED;
			ThisEvent.EventParam = CurrentMode;
			PostLEDService(ThisEvent);
			// Post an event to the reset service to wake it up
			ThisEvent.EventType = ES_INTERACTION;
			PostResetService(ThisEvent);
			// Reset the constant counter
			VoltageConstantTicks = 0;
		}
			
		// Update the LastADC variable
		LastADCState = CurrentADCState;
		
		// Start the timer again
		ES_Timer_InitTimer(RESISTIVE_STRIP_TIMER, SAMPLING_INTERVAL);
	
	}
  return ReturnEvent;
}




/****************************************************************************
 Function
    getCurrentMode

 Parameters
   CurrentVoltage. ADC voltage from 0 to 4095

 Returns
   uint32_t Mode. Return a mode from 0-9

****************************************************************************/
uint8_t getCurrentMode(uint16_t CurrentVoltage) {
	uint8_t mode;
	if (CurrentVoltage <= 400){ 
		mode = 0;
	} else if (CurrentVoltage <= 800){ 
		mode = 1;
	} else if (CurrentVoltage <= 1200){ 
		mode = 2;
	} else if (CurrentVoltage <= 1600){ 
		mode = 3;
	} else if (CurrentVoltage <= 2000){ 
		mode = 4;
	} else if (CurrentVoltage <= 2400){ 
		mode = 5;
	} else if (CurrentVoltage <= 2800){ 
		mode = 6;
	} else if (CurrentVoltage <= 3200){ 
		mode = 7;
	} else if (CurrentVoltage <= 3800){ 
		mode = 8;
	} else { // 4096 
		mode = 9; 
	}
	return mode;
}


/****************************************************************************
 Function
    getADCState

 Parameters
   None

 Returns
   uint32_t Voltage. The voltage from the resistive strip on a 0-4095 scale

****************************************************************************/
uint32_t getADCState(void) {
	uint32_t ADInput[4];
	uint32_t CurrentInput;
	ADC_MultiRead(ADInput);
	CurrentInput = ADInput[1]; // Get ADC data from PE1
	return CurrentInput;
}



/****************************************************************************
 Function
    getADCDiff

 Parameters
   CurrentADCState,LastADCState

 Returns
   uint32_t Voltage. Return the difference between the previous ADC state and the current one

****************************************************************************/
uint32_t getADCDiff(uint32_t CurrentADCState, uint32_t LastADCState) {
	if (CurrentADCState >= LastADCState) {
		return (CurrentADCState - LastADCState);
	} else {
		return (LastADCState - CurrentADCState);
	}
}
