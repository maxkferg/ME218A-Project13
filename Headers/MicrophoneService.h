/********************************************************************

  Header file MicrophoneService

 *********************************************************************/


#ifndef MicrophoneService_H
#define MicrophoneService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "kiss_fft.h"

// Public Function Prototypes
bool InitMicrophoneService ( uint8_t Priority );
bool PostMicrophoneService( ES_Event ThisEvent );
bool CheckMicrophoneEvents( void );
ES_Event RunMicrophoneService( ES_Event ThisEvent );

typedef enum { MicrophoneInitState,
							 MicrophoneFourierState,
							 MicrophoneResetState
} MicrophoneState_t ;


#endif /* MicrophoneService_H */

