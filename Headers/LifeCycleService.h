/********************************************************************

  Header file LifecycleService

 *********************************************************************/


#ifndef LifecycleService_H
#define LifecycleService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitPState,
							 WelcomeState,
               WaitingState,
               FinaleState
} LifecycleState_t ;

// Public Function Prototypes
bool InitLifecycleService ( uint8_t Priority );
bool PostLifecycleService ( ES_Event ThisEvent );
ES_Event RunLifecycleService ( ES_Event ThisEvent );
void PostLifecycleEventGenerator( ES_Event ThisEvent );


#endif /* LifecycleService_H */

