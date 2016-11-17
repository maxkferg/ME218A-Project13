/********************************************************************

  Header file WaterTube

 *********************************************************************/


#ifndef WaterTubeService_H
#define WaterTubeService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// Public Function Prototypes
bool InitWatertubeService ( uint8_t Priority );
bool PostWatertubeService( ES_Event ThisEvent );
ES_Event RunWatertubeService( ES_Event ThisEvent );

typedef enum { WaterInitState,
	             WaterDisplayState,
               WaterResetState
} WatertubeState_t ;

#endif /* WaterTubeService_H */

