/****************************************************************************
 Module
   MicrophoneService.c

 Revision
   1.0.1

 Description
   Handle the input from the Microphone
	 Perform a FFT on the signal
	 Post to the LED and water tube services

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/26/16 09:58 jec      filled in missing code for Lab 3
 10/19/16 13:24 jec      added comments about where to add deferal and recall
 01/12/15 21:47 jec      converted to LCD module for lab 3
 11/02/13 17:21 jec      added exercise of the event deferral/recall module
 08/05/13 20:33 jec      converted to test harness service
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"


#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

// Bit definitions
#include "ALL_BITS.h"

// Includes for Fourier Math
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "kiss_fft.h"

#include "ADMulti.h"

// Include services we need to post to
#include "WatertubeService.h"

// Include my own header
#include "MicrophoneService.h"

#ifndef M_PI
#define M_PI 3.14159265358979324
#endif

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)

#define N 32
#define MICROPHONE_PIN 0 
#define SAMPLING_PERIOD 100 // 100 microseconds -> 8000Hz


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service. They should be functions
   relevant to the behavior of this service
*/
static void PrintAudioBuffer( void );
static void PrintFourierBuffer( void );
static void PrintAverageBuffer( void );
static void PushAudioBuffer(float newValue);
static void PushAverageBuffer( void );
static float SumFourierOutputs(uint16_t Start, uint16_t End);
static float GetWaterHeight(uint8_t WaterTubeNumber, float Sensitivity);
static void PerformFFT(void);
	
static void TestFft(const char* title, const kiss_fft_cpx in[N], kiss_fft_cpx out[N]);
static void RunFFTTest(void);
static float square(float b);
static float max(float a, float b);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static uint8_t CurrentState;

// We keep a buffer of N values to perform the FFT over
// New values are pushed to the front of the array and any overflow is discarded
// After performing the fourier transform we are left with N FourierOutput values
static kiss_fft_cpx AudioBuffer[N];
static kiss_fft_cpx FourierOutput[N];
static float AverageBuffer[N/2];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     MicrophoneService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
****************************************************************************/
bool InitMicrophoneService( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;

	puts("Intializing the Microphone\r\n");

	//Put us into the initial pseudo-state to set up for the initial transition
	CurrentState = MicrophoneInitState;
	
	ADC_MultiInit(4);

	// Run a quick test to make sure the FFT logic works correctly
	//RunFFTTest();

	// Set up the short timer for inter-command timings
  ES_ShortTimerInit(MyPriority, SHORT_TIMER_UNUSED);

  // Post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
    return true;
  } else {
     return false;
  }
}



/****************************************************************************
 Function
     PostMicrophoneService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
****************************************************************************/
bool PostMicrophoneService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}


/****************************************************************************
 Function
    RunMicrophoneService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunMicrophoneService( ES_Event ThisEvent )
{
	float Sensitivity = 32;
	ES_Event WaterTubeEvent;
	ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	uint32_t ADInput[4];
	static uint8_t SampleCounter;
	static uint8_t FourierCounter;
	float intensity = 0;	
	
	switch(CurrentState){
		case MicrophoneInitState:
			// This state prepares everythin for sampling
			// We wait for the MICROPHONE_START event before starting
			if (ThisEvent.EventType==MICROPHONE_START){
				printf("Microphone: Enagaging the Microphone\r\n");
				CurrentState = MicrophoneWaitForSample;
				ES_ShortTimerStart(TIMER_A,SAMPLING_PERIOD);
			} else {
				printf("Microphone: Waiting for start command\r\n");
				printf("Microphone: Press 'm' to start and then 'n' to stop\r\n");
			}		
			break;

		case MicrophoneWaitForSample:
			// In this state we are waiting to take another sample
			// We stay in this state whenever possible to maximize sampling rate
			// If this is a Timout we sample again
			if (ThisEvent.EventType==MICROPHONE_STOP){
				CurrentState = MicrophoneWaitForSample;
				PostMicrophoneService(ThisEvent);
			}
			if (ThisEvent.EventType==ES_SHORT_TIMEOUT){
				// Read the microphone port
				ADC_MultiRead(ADInput);

				// Increase the count
				SampleCounter+=1;

				// Normalize to [0, 1]
				intensity = (float)ADInput[MICROPHONE_PIN]; ;
				intensity = (intensity/4096);

				// Push the new value to the audio buffer 
				PushAudioBuffer(intensity);
		
				// If we have collected 32 samples we can perform the FFT
				// Change the state to stop collecting, and post a MICROPHONE_SOUND_RECORDED event
				if ( SampleCounter % 32 == 0 ){
					ES_Event CurrentEvent;
					CurrentState = MicrophoneFourierState;
					CurrentEvent.EventType = MICROPHONE_SOUND_RECORDED;
					CurrentEvent.EventParam = 0;
					PostMicrophoneService(CurrentEvent);
					SampleCounter = 0;
				} else{
					// Sample again soon
					ES_ShortTimerStart(TIMER_A,SAMPLING_PERIOD);
				}
			}
			break;
 			
		case MicrophoneFourierState:
			// In this state we are calculating new fourier values
		  // After this state we either start sampling again, or go
		  // to the MicrophoneWaterState
			printf(".");
			
			if (ThisEvent.EventType==MICROPHONE_SOUND_RECORDED){
				// Calculate the FFT.
				// Input will be AudioBuffer
				// Output will be FourierOutputs
				PerformFFT();
				PushAverageBuffer();
				FourierCounter++;
				// Decide which state to move to
				if (FourierCounter>50){
					// Move to the Water state
					ES_Event CurrentEvent;
					CurrentState = MicrophoneWaterState;
					CurrentEvent.EventType = MICROPHONE_FOURIER_COMPLETED;
					PostMicrophoneService(CurrentEvent);
					FourierCounter = 0;
				} else {
					// Default: Move back to the sampling state
					CurrentState = MicrophoneWaitForSample;
					ES_ShortTimerStart(TIMER_A,SAMPLING_PERIOD);
				}
			}
		break;
		

		case MicrophoneWaterState:
			// In this state we take a break from the hard work and post 
		  // our results. We have finished sampling and transforming at this point
		
			if (ThisEvent.EventType==MICROPHONE_FOURIER_COMPLETED){
				PrintAudioBuffer();
				PrintFourierBuffer();
				PrintAverageBuffer();
					
				// Water tube 1
				WaterTubeEvent.EventType = CHANGE_WATER_1;
				WaterTubeEvent.EventParam = GetWaterHeight(1, Sensitivity);
				PostWatertubeService(WaterTubeEvent);

				// Water tube 2					
				WaterTubeEvent.EventType = CHANGE_WATER_2;
				WaterTubeEvent.EventParam = GetWaterHeight(2, Sensitivity);
				PostWatertubeService(WaterTubeEvent);
				
				// Water tube 3
				WaterTubeEvent.EventType = CHANGE_WATER_3;
				WaterTubeEvent.EventParam = GetWaterHeight(3, Sensitivity);
				PostWatertubeService(WaterTubeEvent);
				
				// Water tube 4
				WaterTubeEvent.EventType = CHANGE_WATER_4;
				WaterTubeEvent.EventParam = GetWaterHeight(4, Sensitivity);
				PostWatertubeService(WaterTubeEvent);
				
				// Water tube 5
				WaterTubeEvent.EventType = CHANGE_WATER_5;
				WaterTubeEvent.EventParam = GetWaterHeight(5, Sensitivity);
				PostWatertubeService(WaterTubeEvent);
				
				// Water tube 6
				WaterTubeEvent.EventType = CHANGE_WATER_6;
				WaterTubeEvent.EventParam = GetWaterHeight(6, Sensitivity);
				PostWatertubeService(WaterTubeEvent);
				
				// Water tube 7
				WaterTubeEvent.EventType = CHANGE_WATER_7;
				WaterTubeEvent.EventParam = GetWaterHeight(7, Sensitivity);
				PostWatertubeService(WaterTubeEvent);
				
				// Now we are going to start sampling again
				printf("Microphone sample complete\r\n");
				CurrentState = MicrophoneInitState;
			}
		break;
	}
  return ReturnEvent;
}

/****************************************************************************
 Function
     GetWaterHeight

 Parameters
     unit8_t FourierTransform[N] - A pointer to the fourier result
		 unit8_t WaterTubeNumber - Number of water tube to calculate for 
     unit8_t Sensitivity  - Twiddle factor for amplitude
		
 Returns
      unit8_t The height of the water tube on a 0 to 10 scale

 Description
     Posts an event to this state machine's queue
****************************************************************************/
static float GetWaterHeight(uint8_t WaterTubeNumber, float Sensitivity){
	switch(WaterTubeNumber){
		// Depending on the water tube number we sum over different frequency ranges
		// Water tube 1
		case 1:
			printf("Sensitivity %f",Sensitivity);
			printf("Fourier %f",SumFourierOutputs(0,2));
			return Sensitivity*(SumFourierOutputs(0,2));
	
		// Water tube 2
		case 2:
			return Sensitivity*(SumFourierOutputs(2,4));

		// Water tube 3
		case 3:
			return Sensitivity*(SumFourierOutputs(4,6));

		// Water tube 4			
		case 4:
			return Sensitivity*(SumFourierOutputs(6,8));

		// Water tube 5
		case 5:
			return Sensitivity*(SumFourierOutputs(8,10));

		// Water tube 6
		case 6:
			return Sensitivity*(SumFourierOutputs(12,14));

		// Water tube 7
		case 7:
			return Sensitivity*(SumFourierOutputs(14,16));
	}
	printf("Unknown tube %i",WaterTubeNumber);
	return 0;
}



/****************************************************************************
 Function
    PrintAudioBuffer

	Description
		Print the values in the Audio Buffer
****************************************************************************/
static void PrintAudioBuffer(){
	// Shift all items right by one
	printf("\r\nA[");
	for (int k=0; k<N; k++){   
    printf("%.2f,",AudioBuffer[k].r);
	}
	// Append the new value
	printf("]\r\n");
}



/****************************************************************************
 Function
    PrintFourierOutput

	Description
		Print the values in the Fourier Coefficients
****************************************************************************/
static void PrintFourierBuffer(){
	printf("F[");
	for (int k=0; k<N; k++){   
    printf("%.2f,",FourierOutput[k].r);
	}
	printf("]\r\n\r\n");
	
	printf("S[");
	for (int k=1; k<(N/2); k++){   
    printf("%.2f,",square(FourierOutput[k].r) + square(FourierOutput[k].i));
	}
	printf("]\r\n");
}



/****************************************************************************
 Function
    PrintFourierOutput

	Description
		Print the values in the Fourier Coefficients
****************************************************************************/
static void PrintAverageBuffer(){
	// Shift all items right by one
	printf("X[");
	for (int k=0; k<N/2; k++){   
    printf("%i,",(int)AverageBuffer[k]);
	}
	printf("]\r\n");
}


/****************************************************************************
 Function
    PushAudioBuffer

	Description
		Push a single value to the audio buffer
	  The last value in the buffer will be discarded
****************************************************************************/
static void PushAudioBuffer(float newValue){
	// Shift all items right by one
	static uint32_t count = 0;
	//newValue = sin(0.7 * M_PI * 4 * count / N);
	
	for (int k=N-1; k > 0; k--){   
    AudioBuffer[k].r = AudioBuffer[k-1].r;
		AudioBuffer[k].i = 0;
	}
	// Append the new value
	AudioBuffer[0].r = newValue;
	count+=1;
}



/****************************************************************************
 Function
    PushAverageBuffer

	Description
		Copy the Fourier magnitudes over to the Average Buffer
		Normalize the AverageBuffer so that the maximum value is 100
****************************************************************************/
static void PushAverageBuffer(void){
	float squared = 0;
	float maximum = 0;
	// Add the squared magnitude onto the buffer [float math]
	for (int k=1; k < N/2; k++){  
			squared = square(FourierOutput[k].r) + square(FourierOutput[k].i);
			if (squared<0 || squared>1000){
				squared = 0;
			}
		  AverageBuffer[k] += squared;
	}
	// Find the maxumim value [float math]
	for (int k=1; k < N/2; k++){  
			maximum = max(maximum,AverageBuffer[k]);
	}
	
	// Normalize everything to 100 [float math]
	for (int k=1; k < N/2; k++){  
			//printf("%i: 100*%f/%f = %f\r\n",k,AverageBuffer[k],maximum,100*AverageBuffer[k]/maximum);
			AverageBuffer[k] = 128*AverageBuffer[k]/maximum;
	}
}



/****************************************************************************
 Function
     SumFourierOutputs

 Parameters
     unit16_t Start - The index to start summing 
		 unit16_t End - Number of water tube to calculate for 

 Returns
      unit8_t The sum of the points from the Start to End non-inclusive 

 Description
     Return the sum of the points from the Start to End non-inclusive 
****************************************************************************/
static float SumFourierOutputs(uint16_t Start, uint16_t End){
	// Shift all items right by one
	float Sum = 0;
	for (int i=Start; i < End; i++){   
    Sum += AverageBuffer[i];
	}
	return Sum;
}




/****************************************************************************
 Function
     PerformFFT

 Parameters
     unit16_t Start - The index to start summing 
		 unit16_t End - Number of water tube to calculate for 

 Returns
      unit8_t The sum of the points from the Start to End non-inclusive 

 Description
     Return the sum of the points from the Start to End non-inclusive 
****************************************************************************/
static void PerformFFT(void){
	// cfg is a type defined by kiss_fourier to manage state
  kiss_fft_cfg cfg;

	//printf("Start Fourier Transform\r\n");

	cfg = kiss_fft_alloc(N, 0, NULL, NULL);
  if (cfg != NULL)
  {
		// Perform the transform
    kiss_fft(cfg, AudioBuffer, FourierOutput);
  } else {
		printf("FOURIER TRANSFORM FAILED?\n");
    printf("NOT ENOUGH MEMORY?\n");
  }	
	//printf("End Fourier Transform\r\n");
}





/****************************************************************************
 Function
    RunFFTTest

	Description
		Run the FFT tests
****************************************************************************/
static void RunFFTTest(void){
	printf("************************************\r\n");
	printf("********** TESTING FFT **************\r\n");
	printf("************************************\r\n");	
	
	kiss_fft_cpx in[N], out[N];
  size_t i;

  for (i = 0; i < N; i++)
    in[i].r = in[i].i = 0;
  TestFft("Zeroes (complex)", in, out);

  for (i = 0; i < N; i++)
    in[i].r = 1, in[i].i = 0;
  TestFft("Ones (complex)", in, out);

  for (i = 0; i < N; i++)
    in[i].r = sin(2 * M_PI * 4 * i / N), in[i].i = 0;
  TestFft("SineWave (complex)", in, out);

	printf("*********************************\r\n");
	printf("******* AND WE'RE DONE **********\r\n");
	printf("*********************************\r\n");	
}


static void TestFft(const char* title, const kiss_fft_cpx in[N], kiss_fft_cpx out[N])
{
  kiss_fft_cfg cfg;

  printf("%s\n", title);

  if ((cfg = kiss_fft_alloc(N, 0, NULL, NULL)) != NULL)
  {
    size_t i;

    kiss_fft(cfg, in, out);
    free(cfg);

    for (i = 0; i < N; i++)
      printf(" in[%2zu] = %+f , %+f    "
             "out[%2zu] = %+f , %+f\r\n",
             i, in[i].r, in[i].i,
             i, out[i].r, out[i].i);
  }
  else
  {
    printf("not enough memory?\n");
  }
}

/****************************************************************************
 Function
    Square

	Description
		Square a pin
****************************************************************************/
float square(float b)
{
    return b*b;
}


/****************************************************************************
 Function
    Max

	Description
		Return the maximum of two floats
****************************************************************************/
static float max(float a, float b)
{
	if (a>b){
		return a;
	} else {
		return b;
	}
}














