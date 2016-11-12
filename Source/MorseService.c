/****************************************************************************
 Module
   MorseElementService.c

 Revision
   1.0.1

 Description
   This is the first service for the Tes

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

#include "MorseService.h"
#include "MorseDecode.h"

#include "BITDEFS.H"
#include "ALL_BITS.h"

/*----------------------------- Module Defines ----------------------------*/
#define VERBOSE 0
#define MORSE_PIN GPIO_PIN_3
#define MORSE_BIT_HI GPIO_PIN_3
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static MorseState_t CurrentState;

// Capture the last input state of the MorseLine
static uint8_t LastInputState;

// Variable for capturing delays between rise/falls 
static uint8_t FirstDelta;

// Intialize all the time measures
static uint16_t TimeOfLastRise;
static uint16_t TimeOfLastFall;
static uint16_t LengthOfDot;

/****************************************************************************
 Function
     InitMorseHardware

 Parameters
     None

 Returns
     Void

 Description
     Initialize the Tiva to receive bits on port B (3)
		 Some code may be duplicated with the shift register code,
		 but it is included here for completeness and modularity
 Notes

 Author
     M Ferguson, 11/5/16, 10:00
****************************************************************************/
static void InitMorseHardware(void){
	// SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN	| SYSCTL_XTAL_16MHZ);
	// Turn on port B and wait a few clock cycles until its on
	puts("Initializing Morse Hardware\r\n");
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
	int Dummy = HWREG(SYSCTL_RCGCGPIO);
	
	// Set port B to digital, and set the direction to IN
	puts("Initializing Morse Hardware\r\n");
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (MORSE_PIN);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= ~(MORSE_PIN);
	puts("Initialized Morse Hardware\r\n");
	
	// Set port A to digital, and set the direction to OUT
	// puts("Initializing Morse Hardware\r\n");
	// HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (MORSE_PIN);
	// HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= ~(MORSE_PIN);
	// puts("Initialized Morse Hardware\r\n");

   // Turn on Calibration LED
   // HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= ALL_BITS;
}


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMorseService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service

 Author
     M Ferguson, 11/5/16, 10:00
****************************************************************************/
bool InitMorseService( uint8_t Priority ){
	puts("Initializing MorseService\r\n");
  ES_Event ThisEvent;	
	
	// Initialize the MyPriority variable with the passed in parameter.
  MyPriority = Priority;
 
	// Initialize the port line to receive Morse code
	InitMorseHardware();

	// Sample port line and use it to initialize the LastInputState variable
	LastInputState = HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS));
	
  // Set up the Deferral Queue
  // ES_InitDeferralQueueWith(DeferralQueue,DeferralQueueLength);
	// Put us into the initial pseudo-state to set up for the initial transition

	// Set CurrentState to be InitMorseElements
	CurrentState = MorseInitPState;
	
	// Set FirstDelta to 0
	FirstDelta = 0;
	
	// Post Event ES_Init to MorseElements queue
	//puts("Posting transition event\r\n");
    ThisEvent.EventType = ES_INIT;
  
  
	if (ES_PostToService( MyPriority, ThisEvent) == true) {
    return true;
  } else {
     return false;
  }
}


/****************************************************************************
 Function
     PostMorseService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     Max Ferguson, 11/5/16, 19:25
****************************************************************************/
bool PostMorseService( ES_Event ThisEvent )
{
	//puts("Posting to morse service\r\n");
	return ES_PostToService(MyPriority, ThisEvent);
}


/*********************************************************************
 Function
   CheckMorseEvents

 Description
   An event checker for incoming Morse code messages
   Returns a MORSE_HI_EVENT when the input goes HI and
   a MORSE_LO_EVENT when the input goes LO.

**********************************************************************/
bool CheckMorseEvents(void)
{
	uint8_t CurrentInputStatus;
	ES_Event CurrentEvent;
	bool isEvent = false;

	CurrentInputStatus = HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) & (MORSE_BIT_HI); 
	
	// Skip the event if nothing has changed
	if ( CurrentInputStatus == LastInputState ){
		return isEvent;
	}
	
	// Some event has occured
	if (CurrentInputStatus == MORSE_BIT_HI){
		CurrentEvent.EventType = RisingEdge;
		CurrentEvent.EventParam = ES_Timer_GetTime();
		isEvent = true;
	} else {
		CurrentEvent.EventType = FallingEdge;
		CurrentEvent.EventParam = ES_Timer_GetTime();
		isEvent = true;
	}

	// Update the status for next time
	LastInputState = CurrentInputStatus;

	// Post the event to the event system
	PostMorseService(CurrentEvent);

	return isEvent;
}


/****************************************************************************
 Function
    RunMorseService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
	RunMorseElementsSM (implements the state machine for Morse Elements)
  The EventType field of ThisEvent will be one of: 
      ES_Init, RisingEdge, FallingEdge, CalibrationCompleted, EOCDetected, DBButtonDown.
  The parameter field of the ThisEvent will be the time that the event occurred.
  Returns ES_NO_EVENT 

 Author
   Max Ferguson, 11/15/16
****************************************************************************/
ES_Event RunMorseService( ES_Event ThisEvent )
{
	//Returns ES_NO_Event
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	MorseState_t NextState;

	//Set NextState to CurrentState
	NextState = CurrentState;

	//Based on the state of the CurrentState variable choose one of the following blocks of code:
	switch (CurrentState){
		// CurrentState is InitMorseElements
		case MorseInitPState:
			// 	If ThisEvent is ES_Init	
			if ( ThisEvent.EventType == ES_INIT ){
				// Set NextState to CalWaitForRise
				NextState = CalWaitForRise;
			}
			//End InitMorseElements block
		break;
		
		// CurrentState is CalWaitForRise
		case CalWaitForRise:
			// If ThisEvent is RisingEdge
			//puts("Rise (Calibration)\r\n");
			if (ThisEvent.EventType == RisingEdge){
				// Set TimeOfLastRise to Time from event parameter
				TimeOfLastRise = ThisEvent.EventParam;
				// Set NextState to CalWaitForFall
				NextState = CalWaitForFall;
			} 
			// If ThisEvent is CalibrationComplete
			if (ThisEvent.EventType == CalCompleted){
				//Set NextState to EOC_WaitRise
				NextState = EOC_WaitRise;
			}
		break; // End CalWaitForRise block

		case CalWaitForFall:
			//If ThisEvent is FallingEdge
			//puts("Fall (Calibration)\r\n");
			if (ThisEvent.EventType == FallingEdge){
					// Set TimeOfLastFall to Time from event parameter
					TimeOfLastFall = ThisEvent.EventParam;
					// Set NextState to CalWaitForRise
					NextState = CalWaitForRise;
					// Call TestCalibration function
					TestCalibration();
			}
		 	//End CalWaitForFall block
		break;

		case EOC_WaitRise:
			// If ThisEvent is RisingEdge
			//puts("EOC_WaitRise");
			if (ThisEvent.EventType == RisingEdge){
				// Set TimeOfLastRise to Time from event parameter
				TimeOfLastRise = ThisEvent.EventParam;
				// Set NextState to EOC_WaitFall
				NextState = EOC_WaitFall;
				// Call CharacterizeSpace function
				CharacterizeSpace();
			} 
			//If ThisEvent is DBButtonDown
			if (ThisEvent.EventType == ButtonDown){
				// Set NextState to CalWaitForRise
				NextState = CalWaitForRise;
				//Set FirstDelta to 0
				FirstDelta = 0;
				// Let the user know that we are recalibrating
				puts("\r\n ------- Recalibration Started ------- \r\n");
				// Turn on Calibration LED
			    // HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= ALL_BITS;
			} 
		break; //End EOC_WaitRise block

		//CurrentState is EOC_WaitFall
		case EOC_WaitFall:
			// ThisEvent is FallingEdge
			//puts("EOC_WaitFall");
			if (ThisEvent.EventType == FallingEdge){
				// Set TimeOfLastFall to Time from event parameter
				TimeOfLastFall = ThisEvent.EventParam;
				//Set NextState to EOC_WaitRise
				NextState = EOC_WaitRise;
				// Debugging
				if (VERBOSE){
					//printf("EOC_WaitFall: EOCDetected");
				}
			} // EndIf
			//If ThisEvent is DBButtonDown
			if (ThisEvent.EventType == ButtonDown){
			// Set NextState to CalWaitForRise
				NextState = CalWaitForRise;
				// Set FirstDelta to 0
				FirstDelta = 0;
			} //Endif 
			// If ThisEvent is EOCDetected
			if (ThisEvent.EventType == EOCDetected){
				// Set NextState to DecodeWaitFall
				NextState = DecodeWaitFall;
				// EOC_WaitFall
				if (VERBOSE){
					//printf("EOC_WaitFall: EOCDetected");
				}
			} // Endif 
		break; // End EOC_WaitFall block

		// CurrentState is DecodeWaitRise
		case DecodeWaitRise:
			// If ThisEvent is RisingEdge
			if (ThisEvent.EventType == RisingEdge){
				//Set TimeOfLastRise to Time from event parameter
				TimeOfLastRise = ThisEvent.EventParam;
				//Set NextState to DecodeWaitFall
				NextState = DecodeWaitFall;
				// Call CharacterizeSpace function
				CharacterizeSpace();
				// Debugging
				if (VERBOSE){
					//printf("DecodeWaitRise: RisingEdge\r\n");
				}
			} //Endif 
			//If ThisEvent is DBButtonDown
			if (ThisEvent.EventType == ButtonDown){
				// Set NextState to CalWaitForRise
				NextState = CalWaitForRise;
				// Set FirstDelta to 0
				FirstDelta = 0;
				// Let the user know that we are recalibrating
				puts("\r\n ------- Recalibration Started ------- \r\n");
				// Turn on Calibration LED
			    // HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= ALL_BITS;
			}	
		break; //End DecodeWaitRise block
			
		// CurrentState is DecodeWaitFall
		case DecodeWaitFall:	
			// If ThisEvent is FallingEdge
			if (ThisEvent.EventType == FallingEdge){
				//Set TimeOfLastFall to Time from event parameter
				TimeOfLastFall = ThisEvent.EventParam;
				// Set NextState to DecodeWaitRise
				NextState = DecodeWaitRise;
				// Call CharacterizePulse function
				CharacterizePulse();
				// Debugging
				if (VERBOSE){
					//printf("DecodeWaitFall: FallingEdge\r\n");
				}
			} //Endif 
			// If ThisEvent is DBButtonDown
			if (ThisEvent.EventType == ButtonDown){
				// Set NextState to CalWaitForRise
				NextState = CalWaitForRise;
				// Set FirstDelta to 0
				FirstDelta = 0;
				// Let the user know that we are recalibrating
				puts("\r\n ------- Recalibration Started ------- \r\n");
				// Turn on Calibration LED
			    // HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= ALL_BITS;
			} // Endif 
			// End DecodeWaitFall block
		break;
	}
	//Set CurrentState to NextState
	CurrentState = NextState;
	return ReturnEvent;
}
	



/****************************************************************************
 Function
     TestCalibration

 Parameters
     Takes no parameters

 Returns
     Returns nothing

 Description
     Posts an event to this state machine's queue

 Author
     Max Ferguson, 11/5/16, 19:25
****************************************************************************/
void TestCalibration(void){
	// Local variable SecondDelta
	static uint16_t SecondDelta;
	ES_Event Event2Post;
	
	// If calibration is just starting (FirstDelta is 0)
	if (FirstDelta == 0){
		// Set FirstDelta to most recent pulse width
		FirstDelta = TimeOfLastFall - TimeOfLastRise;
	} else {
		// Set SecondDelta to most recent pulse width
		SecondDelta = TimeOfLastFall - TimeOfLastRise;
		//If (100.0 * FirstDelta / SecondDelta) less than or equal to 33.33
		if ((100.0 * FirstDelta / SecondDelta) <= 33.66){
			// Save FirstDelta as LengthOfDot
			LengthOfDot = FirstDelta;
			// PostEvent CalCompleted to MorseElementsSM
			//printf("LENGTH OF DOT %d",LengthOfDot);
			Event2Post.EventType = CalCompleted;
			PostMorseService(Event2Post);
			printf("Length of dot %d",LengthOfDot);
			puts("\r\n ------- Calibration Completed ------- \r\n");
			// Turn OFF Calibration LED
			// HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) &= 0;
		} 
		// ElseIf (100.0 * FirstDelta / Second Delta) greater than 300.0
		else if ((100.0 * FirstDelta/SecondDelta) > 299.0){
			// Save SecondDelta as LengthOfDot
			LengthOfDot = SecondDelta;
			// PostEvent CalCompleted to MorseElementsSM
			Event2Post.EventType = CalCompleted;
			PostMorseService(Event2Post);
			printf("Length of dot %d",LengthOfDot);
			printf("\r\n ------- Calibration Completed ------- \r\n");
			// Turn OFF Calibration LED
			// HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) &= 0;
		// Else (prepare for next pulse)
		} else { 
			// Set FirstDelta to SecondDelta
			FirstDelta = SecondDelta;
		}
	}
	//Return 
	return;
}
 
		
		
/****************************************************************************
 Function
     CharacterizeSpace

 Parameters
     none

 Returns
     nothing

 Description
     Posts one of EOCDetected Event, EOWDetected Event, BadSpace Event as appropriate
	 on good dot-space, does nothing

****************************************************************************/

void CharacterizeSpace(void){
	//Local variable LastInterval, Event2Post
	uint16_t LastInterval;
	ES_Event Event2Post;
	
	// Calculate LastInterval as TimeOfLastRise – TimeOfLastFall
	LastInterval = TimeOfLastRise - TimeOfLastFall;
	//printf("Space Width %d\r\n",LastInterval);
	
	// If LastInterval OK for a Dot Space
	if (LastInterval > (LengthOfDot-6) && LastInterval < (LengthOfDot+6)){
			if (VERBOSE){
				printf("Dot Space Detected");
			}
	} else {
		// If LastInterval OK for a Character Space
		// A character space is 5 dot times long
		if (LastInterval>=(3*LengthOfDot-52) && LastInterval<=(3*LengthOfDot+52)) {
			// PostEvent EOCDetected Event to Decode Morse Service & Morse Elements Service
			Event2Post.EventType = EOCDetected;
			Event2Post.EventParam = ES_Timer_GetTime();
			PostMorseDecodeService(Event2Post);
			PostMorseService(Event2Post);
			if (VERBOSE){
				printf("Character Space Detected");
			}
		} else {	
			// If LastInterval OK for Word Space
			// A word space is 7 dot times long
			if (LastInterval >= (7*LengthOfDot-122) && LastInterval <= (7*LengthOfDot+122)){
				// PostEvent EOWDetected Event to Decode Morse Service
				Event2Post.EventType = EOWDetected;
				Event2Post.EventParam = ES_Timer_GetTime();
				PostMorseDecodeService(Event2Post);
				if (VERBOSE){
					printf("Character Space Detected");
				}
			} else {
				// PostEvent BadSpace Event to Decode Morse Service
				Event2Post.EventType = BadSpace;
				Event2Post.EventParam = ES_Timer_GetTime();
				PostMorseDecodeService(Event2Post);
				if (VERBOSE){
					printf("Bad Space Detected");
				}
			}
		}
	}
	// Return
	return;
} // End of CharacterizeSpace
			
			
		
		
/****************************************************************************
 Function
     CharacterizePulse

 Parameters
     Takes no parameters

 Returns
     Returns nothing

 Description
     Posts one of DotDetectedEvent, DashDetectedEvent, BadPulseEvent,

 Author
     Max Ferguson, 11/5/16, 19:25
****************************************************************************/
void CharacterizePulse(void){
	//Local variable LastPulseWidth, Event2Post
	uint16_t LastPulseWidth;
	ES_Event Event2Post;

	// Calculate LastPulseWidth as TimeOfLastFall - TimeOfLastRise
	LastPulseWidth = TimeOfLastFall - TimeOfLastRise;
	printf("Pulse Width %i\r\n",LastPulseWidth);
	//If LastPulseWidth OK for a dot
	if ((LastPulseWidth >= (LengthOfDot-1)) && (LastPulseWidth <= (LengthOfDot+1))){
		// PostEvent DotDetected Event to Decode Morse Service
		Event2Post.EventType = DotDetected;
		PostMorseDecodeService(Event2Post);
		if (VERBOSE){
			printf("Dot Detected");
		}
	} else {
		// If LastPulseWidth OK for dash
		if (LastPulseWidth >= (3*LengthOfDot-10) && LastPulseWidth <= (3*LengthOfDot+10)){
			// PostEvent DashDetected Event to Decode Morse Service
			Event2Post.EventType = DashDetected;
			PostMorseDecodeService(Event2Post);
			if (VERBOSE){
				printf("Dash Detected");
			}
		} else {
			// PostEvent BadPulse Event to Decode Morse Service
			Event2Post.EventType = BadPulse;
			PostMorseDecodeService(Event2Post);
			if (VERBOSE){
				printf("Bad Pulse Detected");
			}
		}
	}
	//Return 
	return;
}
