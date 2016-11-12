/****************************************************************************
 Module
   ButtonDebounce.c

 Revision
   1.0.1

 Description
   Handle reset button presses

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
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "ButtonDebounce.h"
#include "MorseDecode.h"
#include "MorseService.h"


#include "BITDEFS.H"
#include "ALL_BITS.h"

/*----------------------------- Module Defines ----------------------------*/
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)
#define FIVE_SEC (ONE_SEC*5)

#define BUTTON_PIN GPIO_PIN_4
#define BUTTON_DOWN BIT4HI
#define BUTTON_DEBOUNCE 30

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t LastButtonState;
static ButtonState_t CurrentState;

/****************************************************************************
 Function
     InitButtonHardware

 Parameters
     None

 Returns
     Void

 Description
     Initialize the Tiva to recive bits on port B (4)
		 Some code may be duplicated with the shift register code,
		 but it is included here for completeness and modularity
 Notes

 Author
     M Ferguson, 11/5/16, 10:00
****************************************************************************/
static void InitButtonHardware(void){
	// SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN	| SYSCTL_XTAL_16MHZ);
	// Turn on port B and wait a few clock cycles until its on
	puts("Initializing Button Hardware\r\n");
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
	int Dummy = HWREG(SYSCTL_RCGCGPIO);
	
	// Set port B to digital, and set the direction to IN
	puts("Initializing Button Hardware\r\n");
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BUTTON_PIN);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= ~(BUTTON_PIN);
	puts("Initialized Button Hardware\r\n");
}


/****************************************************************************
 Function
     InitButtonDebounce

 Parameters
     Takes a priority number

 Returns
     Returns True

 Description
     Initialize the Tiva to recive bits on port B (3)
		 Some code may be duplicated with the shift register code,
		 but it is included here for completeness and modularity
 Notes

 Author
     M Ferguson, 11/6/16
****************************************************************************/
bool InitButtonDebounceService( uint8_t Priority ){
	puts("Initializing ButtonDebounce\r\n");

	//Initialize the MyPriority variable with the passed in parameter.
	MyPriority = Priority;

	// Initialize the port line to monitor the button
	InitButtonHardware();
	
	// Sample the button port pin and use it to initialize LastButtonState
	LastButtonState = HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) & BUTTON_PIN;

	// Set CurrentState to be DEBOUNCING
	CurrentState = DEBOUNCING;
	
	// Start debounce timer (timer posts to ButtonDebounceSM)
  ES_Timer_InitTimer(BUTTON_TIMER, ONE_SEC);
	//ES_Timer_SetTimer(BUTTON_TIMER, HALF_SEC);

	// Nothing went wrong so we return True
  return true;
}


/****************************************************************************
 Function
     PostButtonService

 Parameters
     EF_Event ThisEvent, the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to the ButtonDebounce state machine's queue
 Author
     Max Ferguson, 11/5/16, 19:25
****************************************************************************/
bool PostButtonDebounceService( ES_Event ThisEvent )
{
	return ES_PostToService(MyPriority, ThisEvent);
}




/*********************************************************************
 Function
   CheckButtonEvents
 
 Parameters
	Takes no parameters
 
 Returns
	Returns True if an event posted

 Description
   An event checker for incoming Morse code messages
   Returns a MORSE_HI_EVENT when the input goes HI and
   a MORSE_LO_EVENT when the input goes LO.

**********************************************************************/
bool CheckButtonEvents(void){
	bool ReturnVal = false;
	ES_Event Event2Post;
	uint8_t CurrentButtonState;

	//Set CurrentButtonState to state read from port pin
	CurrentButtonState = HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) ;
	CurrentButtonState = CurrentButtonState & BUTTON_PIN;
	
	//If the CurrentButtonState is different from the LastButtonState
	if (CurrentButtonState!=LastButtonState){
		//Set ReturnVal = True
		ReturnVal = true;	
		//If the CurrentButtonState is down
		if (CurrentButtonState==BUTTON_DOWN){
			//PostEvent ButtonDown to ButtonDebounce queue
			Event2Post.EventType = ButtonDown;
			Event2Post.EventParam = ES_Timer_GetTime();
			PostButtonDebounceService(Event2Post);
		} else {
			//PostEvent ButtonUp to ButtonDebounce queue
			Event2Post.EventType = ButtonUp;
			Event2Post.EventParam = ES_Timer_GetTime();
			PostButtonDebounceService(Event2Post);		
		}
		//Set LastButtonState to the CurrentButtonState
		LastButtonState = CurrentButtonState;
	}
	
	//Return ReturnVal
	return ReturnVal;
}




/****************************************************************************
 Function
     RunMorseDebounceService

 Parameters
     None

 Returns
     Void

 Description
		Implements the service for Morse Decode.
		The EventType field of ThisEvent will be one of: DotDetectedEvent, DashDetectedEvent, EOCDetected, EOWDetected, ButtonDown.
		Returns ES_NO_EVENT if No Error detected, ES_ERROR otherwise
		local var ReturnValue initialized to ES_NO_EVENT
 Author
     Max Ferguson, 11/5/16, 10:00
****************************************************************************/
ES_Event RunMorseDebounceService(ES_Event ThisEvent){
	ES_Event Event2Post;
	ES_Event ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT;

	// The EventType field of ThisEvent will be one of: ButtonUp, ButtonDown, or ES_TIMEOUT
	switch (CurrentState){
		
		// If CurrentState is Debouncing
		case DEBOUNCING:
			// If EventType is ES_TIMEOUT & parameter is debounce timer number
			if (ThisEvent.EventType==ES_TIMEOUT){
				// Set CurrentState to Ready2Sample
				CurrentState = Ready2Sample;
			}
		break;
			
		// Else if CurrentState is Ready2Sample	
		case Ready2Sample:
			// If EventType is ButtonUp
			if (ThisEvent.EventType==ButtonUp){
				// Start debounce timer
				ES_Timer_InitTimer(BUTTON_TIMER, ONE_SEC);
				// Set CurrentState to DEBOUNCING
				CurrentState = DEBOUNCING;
				// Post DBButtonUp to MorseElements & DecodeMorse queues
				Event2Post.EventType = ButtonUp;
				Event2Post.EventParam = ES_Timer_GetTime();
				PostMorseService(Event2Post);
				PostMorseDecodeService(Event2Post);
			} //End if
			//If EventType is ButtonDown
			if (ThisEvent.EventType==ButtonDown){
				// Start debounce timer
				ES_Timer_InitTimer(BUTTON_TIMER, ONE_SEC);
				// Set CurrentState to DEBOUNCING
				CurrentState = DEBOUNCING;
				// Post DBButtonDown to MorseElements & DecodeMorse queues
				Event2Post.EventType = ButtonDown;
				Event2Post.EventParam = ES_Timer_GetTime();
				PostMorseService(Event2Post);
				PostMorseDecodeService(Event2Post);
				printf("Posted ButtonDown to MorseService and MorseDecode Service\r\n");
			}
		break;
	}
	return ReturnEvent;
}














