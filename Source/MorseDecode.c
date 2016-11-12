/****************************************************************************
 Module
   MorseDecode.c

 Revision
   1.0.1

 Description
   Decode Morse module

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
 */
#include <stdio.h>
#include <string.h>
 
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

#include "MorseDecode.h"
#include "LCDService.h"

#include "BITDEFS.H"
#include "ALL_BITS.h"

/*----------------------------- Module Defines ----------------------------*/
// define the length of the current morse string
#define VERBOSE 0
#define MORSE_STRING_SIZE 8
#define MORSE_CODE_ARRAY_SIZE 47

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890?.,:'-/()\"= !$&+;@_";
char morseCode[][8] ={ ".-","-...","-.-.","-..",".","..-.","--.",
                      "....","..",".---","-.-",".-..","--","-.","---",
                      ".--.","--.-",".-.","...","-","..-","...-",
                      ".--","-..-","-.--","--..",".----","..---",
                      "...--","....-",".....","-....","--...","---..",
                      "----.","-----","..--..",".-.-.-","--..--",
                      "---...",".----.","-....-","-..-.","-.--.-",
                      "-.--.-",".-..-.","-...-","-.-.--","...-..-",
                      ".-...",".-.-.","-.-.-.",".--.-.","..--.-"
                     };
static uint8_t MyPriority;
static char MorseString[MORSE_STRING_SIZE];
static uint8_t MsIndex=0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMorseDecode

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
     M Ferguson, 11/5/16, 10:00
****************************************************************************/
bool InitMorseDecode( uint8_t Priority ){
	puts("Initializing MorseDecode\r\n");

	// Initialize the MyPriority variable with the passed in parameter.
	MyPriority = Priority;
	
	// Clear (empty) the MorseString variable
	ClearMorseString();
	
	// Nothing went wrong so we return True
	return true;
}


/****************************************************************************
 Function
     PostMorseDecode

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
bool PostMorseDecodeService( ES_Event ThisEvent )
{
	//puts("Posting to MORSE decode\r\n");
	return ES_PostToService(MyPriority, ThisEvent);
}


/****************************************************************************
 Function
     RunMorseDecode

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
ES_Event RunMorseDecode(ES_Event ThisEvent){
	ES_Event ReturnEvent;
	ES_Event CharEvent;
	ReturnEvent.EventType = ES_NO_EVENT;

	// If ThisEvent is DotDetected Event
	if (ThisEvent.EventType==DotDetected){
		// If there is room for another Morse element in the internal representation
		if (MsIndex < MORSE_STRING_SIZE){
			// Add a Dot to the internal representation
			MorseString[MsIndex] = '.';
			MsIndex++;
		} else {
			// Set ReturnValue to ES_ERROR with param set to indicate this location
			ReturnEvent.EventType = ES_ERROR;
			ReturnEvent.EventParam = MsIndex;
		}
	} // End if ThisEvent is DotDetected Event
		
	// If ThisEvent is DashDetected Event
	if (ThisEvent.EventType==DashDetected){

		// If there is room for another Morse element in the internal representation
		if (MsIndex<MORSE_STRING_SIZE){
			// Add a Dash to the internal representation
			MorseString[MsIndex] = '-';
			MsIndex++;
		} else {
			// Set ReturnValue to ES_ERROR with param set to indicate this location
			ReturnEvent.EventType = ES_ERROR;
			ReturnEvent.EventParam = MsIndex;
		}
	} // End if ThisEvent is DashDetected Event

	// If ThisEvent is EOCDetected Event
	if (ThisEvent.EventType==EOCDetected){
		if (VERBOSE){
			printf("End of Char Detected\r\n");
		}
		// Print to LCD the decoded character
		CharEvent.EventType = ES_LCD_PUTCHAR;
		CharEvent.EventParam = DecodeMorseString();
		PostLCDService(CharEvent);				
		// Clear (empty) the MorseString variable
		ClearMorseString();
	} // End if is EOCDetected Event


	// If ThisEvent is EOWDetected Event
	if (ThisEvent.EventType==EOWDetected){
		if (VERBOSE){
			printf("End of Word Detected\r\n");
		}
		// Print to LCD the decoded character
		CharEvent.EventType = ES_LCD_PUTCHAR;
		CharEvent.EventParam = DecodeMorseString();
		PostLCDService(CharEvent);	
		// Print to the LCD a space
		CharEvent.EventParam = ' ';
		PostLCDService(CharEvent);	
		// Clear (empty) the MorseString variable
		ClearMorseString();
	} // End if ThisEvent is EOWDetected Event


	// If ThisEvent is BadSpace Event
	if (ThisEvent.EventType==BadSpace){
		if (VERBOSE){
			puts("BadSpace Event");
		}
		// Clear (empty) the MorseString variable
		ClearMorseString();
	} // End if ThisEvent is BadSpace Event


	// If ThisEvent is BadPulse Event
	if (ThisEvent.EventType==BadPulse){
		if (VERBOSE){
			puts("BadPulse Event");
		}
		// Clear (empty) the MorseString variable
		ClearMorseString();
	} // End if ThisEvent is BadPulse Event

	// If ThisEvent is DBButtonDown Event
	if (ThisEvent.EventType==ButtonDown){
		if (VERBOSE){
			puts("ButtonDown Event\r\n");
		}
		// Clear (empty) the MorseString variable
		ClearMorseString();
	} // End if ThisEvent is ButtonDown Event
	
	// Print the Morse String for debugging
	printf("MORSE STRING:");
	PrintMorseString();
	printf("\r\n");
	
	return ReturnEvent;
}


/****************************************************************************
 Function
    DecodeMorseString

 Parameters
    None

 Returns
	Returns either a character or a '~' indicating failure

 Description
    Takes no parameters, returns either a character or a '~' indicating failure

 Author
     M Ferguson, 11/5/16, 10:00
****************************************************************************/
char DecodeMorseString(void){
	char returnLetter;
	// For every entry in the array MorseCode
	for (int i=0; i<MORSE_CODE_ARRAY_SIZE; i++){
		// If MorseString is the same as current position in MorseCode 
		if (strcmp(MorseString, morseCode[i])==0){
			// return contents of current position in LegalChars
			returnLetter = legalChars[i];
			return returnLetter;
		}
	}
	// return '~', since we didn't find a matching string in MorseCode
	returnLetter = '~';
	return returnLetter;
}



/****************************************************************************
 Function
     ClearMorseString

 Parameters
     none

 Returns
	 none

 Description
 	 Clear the MorseString array, and set the index to 0

****************************************************************************/
static void ClearMorseString(void){
	for (int i=0; i<MORSE_STRING_SIZE; i++){
		MorseString[i] = 0;
	}
	MsIndex = 0;
}



/****************************************************************************
 Function
     PrintMorseString

 Parameters
     Takes no parameters

 Returns
     Returns nothing

 Description
     Print the current MorseString to the terminal

 Author
     Max Ferguson, 11/5/16, 19:25
****************************************************************************/
static void PrintMorseString(void){
	for (int i=0; i<MsIndex; i++){
		printf("%c",MorseString[i]);
	}
}


