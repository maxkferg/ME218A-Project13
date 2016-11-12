/********************************************************************

  Header file ButtonService

 *********************************************************************/

#ifndef ButtonDebounce_H
#define ButtonDebounce_H

// Event Definitions
#include "ES_Configure.h"
#include "ES_Types.h" 

// Private functions
static void InitButtonHardware(void);
	

// typedefs for the states
// State definitions for use with the query function
typedef enum { 
	DEBOUNCING,
	Ready2Sample,
} ButtonState_t;

// Public Function Prototypes
bool InitButtonDebounceService( uint8_t Priority );
bool CheckButtonEvents(void);
bool PostButtonDebounceService( ES_Event ThisEvent );
ES_Event RunMorseDebounceService(ES_Event ThisEvent);
	
#endif /* ButtonService_H */

