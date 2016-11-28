/********************************************************************

  Header file KnobService

 *********************************************************************/


#ifndef KnobService_H
#define KnobService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Framework.h"
#include "PWM10Tiva.h"


// Public Function Prototypes
bool InitKnobService ( uint8_t Priority );
bool PostKnobService( ES_Event ThisEvent );
bool CheckKnobEvents( void );
ES_Event RunKnobService( ES_Event ThisEvent );

typedef enum { KnobVibrating,
							 KnobSleeping
} KnobState_t ;


#endif /* KnobService_H */

