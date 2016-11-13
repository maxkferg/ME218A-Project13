/********************************************************************

  Header file SensitivityService.h

 *********************************************************************/


#ifndef SensitivityService_H
#define SensitivityService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { SensitivityInitState,
							 SensitivityMainState,
							 SensitivityResetState
} SensitivityState_t ;


// Public Function Prototypes
bool InitSensitivityService ( uint8_t Priority );
bool PostSensitivityService( ES_Event ThisEvent );
ES_Event RunSensitivityService( ES_Event ThisEvent );


#endif /* SensitivityService_H */

