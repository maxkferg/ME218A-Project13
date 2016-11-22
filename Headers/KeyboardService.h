/********************************************************************

  Header file KeyboardService

 *********************************************************************/


#ifndef KeyboardService_H
#define KeyboardService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitPState,
							 WelcomeState,
               WaitingState,
               FinaleState
} KeyboardState_t ;

// Public Function Prototypes
bool InitKeyboardService ( uint8_t Priority );
bool PostKeyboardService ( ES_Event ThisEvent );
ES_Event RunKeyboardService ( ES_Event ThisEvent );
void PostKeyboardEventGenerator( ES_Event ThisEvent );


#endif /* KeyboardService_H */

