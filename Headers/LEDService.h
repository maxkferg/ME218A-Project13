/********************************************************************

  Header file LEDService.h

 *********************************************************************/
#ifndef LEDService_H
#define LEDService_H
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Types.h"

// State definitions
typedef enum { 
	InitLED,
	Welcome,
	Waiting4ADC
} LEDMode_t ;

// Public Function Prototypes
bool InitLEDService ( uint8_t Priority );
bool PostLEDService( ES_Event ThisEvent );
ES_Event RunLEDService( ES_Event ThisEvent );
bool CheckLEDEvents(void);
#endif 
