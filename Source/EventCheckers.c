/****************************************************************************
 Module
   EventCheckers.c

 Revision
   1.0.1 

 Description
   This is the sample for writing event checkers along with the event 
   checkers used in the basic framework test harness.

 Notes
   Note the use of static variables in sample event checker to detect
   ONLY transitions.
   
 History
 When           Who     What/Why
 -------------- ---     --------
 08/06/13 13:36 jec     initial version
****************************************************************************/


#include "ES_Configure.h"
#include "ES_Events.h"
#include "ES_PostList.h"
#include "ES_ServiceHeaders.h"
#include "ES_Port.h"
#include "EventCheckers.h"

// Import CheckMicrophoneEvents
#include "MicrophoneService.h"


// This is the event checking function sample. It is not intended to be 
// included in the module. It is only here as a sample to guide you in writing
// your own event checkers
#if 0
/****************************************************************************
 Function
   Check4Lock
 Parameters
   None
 Returns
   bool: true if a new event was detected
 Description
   Sample event checker grabbed from the simple lock state machine example
 Notes
   will not compile, sample only
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Lock(void)
{
  static uint8_t LastPinState = 0;
  uint8_t CurrentPinState;
  bool ReturnVal = false;
  
  CurrentPinState =  LOCK_PIN;
  // check for pin high AND different from last time
  // do the check for difference first so that you don't bother with a test
  // of a port/variable that is not going to matter, since it hasn't changed
  if ( (CurrentPinState != LastPinState) &&
       (CurrentPinState == LOCK_PIN_HI) )
  {                     // event detected, so post detected event
    ES_Event ThisEvent;
    ThisEvent.EventType = ES_LOCK;
    ThisEvent.EventParam = 1;
    // this could be any of the service post function, ES_PostListx or 
    // ES_PostAll functions
    ES_PostList01(ThisEvent); 
    ReturnVal = true;
  }
  LastPinState = CurrentPinState; // update the state for next time

  return ReturnVal;
}
#endif

// Wrapper function for 
bool ES_CheckMicrophone( void ){
	return CheckMicrophoneEvents();
}



/*******************************************************************g*********
 Function
   Check4Keystroke
 Parameters
   None
 Returns
   bool: true if a new key was detected & posted
 Description
   checks to see if a new key from the keyboard is detected and, if so, 
   retrieves the key and posts an ES_NewKey event to TestHarnessService0
 Notes
   The functions that actually check the serial hardware for characters
   and retrieve them are assumed to be in ES_Port.c
   Since we always retrieve the keystroke when we detect it, thus clearing the
   hardware flag that indicates that a new key is ready this event checker 
   will only generate events on the arrival of new characters, even though we
   do not internally keep track of the last keystroke that we retrieved.
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Keystroke(void)
{
  if ( IsNewKeyReady() ) // new key waiting?
  {
    ES_Event ThisEvent;
    ThisEvent.EventType = ES_NEW_KEY;
    ThisEvent.EventParam = GetNewKey();
    // test distribution list functionality by sending the 'L' key out via
    // a distribution list.
    if ( ThisEvent.EventParam == 'L'){
      ES_PostList00( ThisEvent );
    }else{   // otherwise post to Service 0 for processing
      puts("This should never be seen");
			//PostTestHarnessService0( ThisEvent );
    }
    return true;
  }
  return false;
}
