/********************************************************************

  Header file FourierService

 *********************************************************************/


#ifndef FourierService_H
#define FourierService_H
#define N 16

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "kiss_fft.h"

// Public Function Prototypes
bool InitFourierService ( uint8_t Priority );
bool PostFourierService( ES_Event ThisEvent );
ES_Event RunFourierService( ES_Event ThisEvent );
void TestFft(const char* title, const kiss_fft_cpx in[N], kiss_fft_cpx out[N]);


#endif /* FourierService_H */

