/********************************************************************

  Header file MorseService.h

 *********************************************************************/

#ifndef MorseDecode_H
#define MorseDecode_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

bool InitMorseDecode( uint8_t Priority );
ES_Event RunMorseDecode(ES_Event ThisEvent);




// typedefs for the states
// State definitions for use with the query function
typedef enum { MorseInitPState, 
							 CalWaitForRise, 
	             CalWaitForFall,
               EOC_WaitRise,
	             EOC_WaitFall,
						   DecodeWaitFall,
	             DecodeWaitRise,
             } MorseState_t ;

// Private functions
static void initMorseHardware( void );
							 
// Public Function Prototypes
bool InitMorseService ( uint8_t Priority );
bool CheckMorseEvents(void);
bool PostMorseService( ES_Event ThisEvent );
void TestCalibration(void);
void CharacterizeSpace(void);
void CharacterizePulse(void);
ES_Event RunMorseService( ES_Event ThisEvent );

#endif /* MorseDecode_H */

