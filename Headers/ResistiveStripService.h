/********************************************************************

  Header file ResistiveStripService.h

 *********************************************************************/


#ifndef ResistiveStripService_H
#define ResistiveStripService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// Public Function Prototypes
bool InitResistiveStripService ( uint8_t Priority );
bool PostResistiveStripService( ES_Event ThisEvent );
ES_Event RunResistiveStripService( ES_Event ThisEvent );


#endif /* ResistiveStripService_H */

