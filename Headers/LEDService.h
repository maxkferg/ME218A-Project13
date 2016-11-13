/********************************************************************

  Header file LEDService.h

 *********************************************************************/


#ifndef LEDService_H
#define LEDService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { LEDInitState,
							 LEDMainState
} LEDState_t ;

// Public Function Prototypes
bool InitLEDService ( uint8_t Priority );
bool PostLEDService( ES_Event ThisEvent );
ES_Event RunLEDService( ES_Event ThisEvent );


#endif /* LEDService_H */

