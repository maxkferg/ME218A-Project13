/********************************************************************

  Header file MorseDecode.h

 *********************************************************************/

#ifndef MorseService_H
#define MorseService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// Private functions

// Public Function Prototypes
bool InitMorseDecode( uint8_t Priority );
bool PostMorseDecodeService( ES_Event ThisEvent );
ES_Event RunMorseDecode(ES_Event ThisEvent);
char DecodeMorseString(void);
static void ClearMorseString(void);
static void PrintMorseString(void);
#endif /* MorseService_H */

