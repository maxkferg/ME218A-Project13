/****************************************************************************
 
  Header file ResetService.h

 ****************************************************************************/
#ifndef ResetService_H
#define ResetService_H
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Types.h"

// State definitions
typedef enum {Running} ResetState_t ;

// Public Function Prototypes
bool InitializeResetService ( uint8_t Priority );
bool PostResetService( ES_Event ThisEvent );
ES_Event RunResetService( ES_Event ThisEvent );
bool CheckButtonEvents(void);
bool CheckInteraction(void);
#endif 
