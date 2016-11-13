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
#include "ADMulti.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

// Bit definitions
#include "ALL_BITS.h"

// Includes for Fourier Mathc
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "kiss_fft.h"

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

#define N 12
#define MICROPHONE_PIN 0 


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service. They should be functions
   relevant to the behavior of this service
*/
static void PrintAudioBuffer();
static void PrintFourierBuffer();
static void PushAudioBuffer(float newValue);
static float SumFourierOutputs(uint16_t Start, uint16_t End);
static float GetWaterHeight(uint8_t WaterTubeNumber, float Sensitivity);
static void PerformFFT(void);
	
static void TestFft(const char* title, const kiss_fft_cpx in[N], kiss_fft_cpx out[N]);
static void RunFFTTest(void);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static uint8_t CurrentState;

static uint8_t LastInputState;

// We keep a buffer of N values to perform the FFT over
// New values are pushed to the front of the array and any overflow is discarded
// After performing the fourier transform we are left with N FourierOutput values
static kiss_fft_cpx AudioBuffer[N];
static kiss_fft_cpx FourierOutput[N];

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

	RunFFTTest();
	
	// TODO: Initialize the port on the tiva

	// Set up the short timer for inter-command timings
  ES_ShortTimerInit(MyPriority, SHORT_TIMER_UNUSED );

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


/*********************************************************************
 Function
   CheckMicrophoneEvents

 Description
   An event checker for incoming Morse code messages
   Returns a MORSE_HI_EVENT when the input goes HI and
   a MORSE_LO_EVENT when the input goes LO.

**********************************************************************/
bool CheckMicrophoneEvents(void)
{
	uint32_t ADInput[4];
	uint8_t CurrentInputStatus;
	ES_Event CurrentEvent;
	bool isEvent = false;
	
	// Read the microphoine port
	ADC_MultiRead(ADInput);
	
	// Check the value on the port
	CurrentInputStatus = ADInput[MICROPHONE_PIN]; 

	if ( CurrentInputStatus > (LastInputState+50) || CurrentInputStatus < (LastInputState-50)){
		// Post the event to the event system	
		 printf("New event with %i\r\n",CurrentInputStatus);
		CurrentEvent.EventType = NEW_SOUND_RECORDED;
		CurrentEvent.EventParam = CurrentInputStatus;
		PostMicrophoneService(CurrentEvent);
		
		// Update LastInputState
		LastInputState = CurrentInputStatus;
		isEvent = true;
	}	

	
	return isEvent;
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
	float Sensitivity = 10.0;
  ES_Event WaterTubeEvent;
	
	ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
	//printf("Microphone service got sound\r\n");

	switch (CurrentState){
		
		case MicrophoneInitState:
			printf("Microphone: InitMState\r\n");
			CurrentState = MicrophoneFourierState;
		break;

		case MicrophoneFourierState:
			// In this state we are pushing new fourier values
			// printf("Microphone: Entered the WaterState\r\n");
			if (ThisEvent.EventType==NEW_SOUND_RECORDED){
				
				// Cast the event parameter to a float
				float intensity = ThisEvent.EventParam;
				
				// Normalize to [-0.5, 0.5]
				intensity = intensity/255-0.5;
				
				// Push the new value to the audio buffer 
				PushAudioBuffer(intensity);
				
				// Calculate the FFT.
				// Input will be AudioBuffer
				// Output will be FouriererOutputs
				// Print the audio buffer
				PrintAudioBuffer();
				PerformFFT();
				PrintFourierBuffer();
			
				// Water tube 1
				WaterTubeEvent.EventType = CHANGE_WATER_1;
				//WaterTubeEvent.EventParam = GetWaterHeight(1, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);

				// Water tube 2					
				WaterTubeEvent.EventType = CHANGE_WATER_2;
				//WaterTubeEvent.EventParam = GetWaterHeight(2, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
				
				// Water tube 3
				WaterTubeEvent.EventType = CHANGE_WATER_3;
				//WaterTubeEvent.EventParam = GetWaterHeight(3, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
				
				// Water tube 4
				WaterTubeEvent.EventType = CHANGE_WATER_4;
				//WaterTubeEvent.EventParam = GetWaterHeight(4, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
				
				// Water tube 5
				WaterTubeEvent.EventType = CHANGE_WATER_5;
				//WaterTubeEvent.EventParam = GetWaterHeight(5, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
				
				// Water tube 6
				WaterTubeEvent.EventType = CHANGE_WATER_6;
				//WaterTubeEvent.EventParam = GetWaterHeight(6, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
				
				// Water tube 7
				WaterTubeEvent.EventType = CHANGE_WATER_7;
				//WaterTubeEvent.EventParam = GetWaterHeight(7, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
				
				// Water tube 8
				WaterTubeEvent.EventType = CHANGE_WATER_8;
				//WaterTubeEvent.EventParam = GetWaterHeight(8, Sensitivity);
				//PostWatertubeService(WaterTubeEvent);
			} else {
				puts("Microphone: Got some other event\r\n");
			}
		break;
	
		case MicrophoneResetState:
			puts("Microphone: Entered the Reset State");
			CurrentState = MicrophoneFourierState;
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
			return Sensitivity*(SumFourierOutputs(0,100));
	
		// Water tube 2
		case 2:
			return Sensitivity*(SumFourierOutputs(100,200));

		// Water tube 3
		case 3:
			return Sensitivity*(SumFourierOutputs(200,300));

		// Water tube 4			
		case 4:
			return Sensitivity*(SumFourierOutputs(300,400));

		// Water tube 5
		case 5:
			return Sensitivity*(SumFourierOutputs(400,500));

		// Water tube 6
		case 6:
			return Sensitivity*(SumFourierOutputs(500,600));

		// Water tube 7
		case 7:
			return Sensitivity*(SumFourierOutputs(600,700));
		// Water tube 8
		case 8:
			return Sensitivity*(SumFourierOutputs(700,N));
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
	printf("[");
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
	// Shift all items right by one
	printf("Real Coefficients\r\n[");
	for (int k=0; k<N; k++){   
    printf("%.2f,",FourierOutput[k].r);
	}
	// Append the new value
	printf("]\r\n");
	
	printf("Imaginary Coefficients\r\n[");
	for (int k=0; k<N; k++){   
    printf("%.2f,",FourierOutput[k].i);
	}
	// Append the new value
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
	for (int k=N-1; k > 0; k--){   
    AudioBuffer[k].r = AudioBuffer[k-1].r;
	}
	// Append the new value
	AudioBuffer[0].r = newValue;
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
	for (int i=Start; i > End; i++){   
    Sum += FourierOutput[i].r;
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

	printf("Start Fourier Transform\r\n");

	cfg = kiss_fft_alloc(N, 0, NULL, NULL);
  if (cfg != NULL)
  {
		// Perform the transform
    kiss_fft(cfg, AudioBuffer, FourierOutput);
  } else {
		printf("FOURIER TRANSFORM FAILED?\n");
    printf("NOT ENOUGH MEMORY?\n");
  }	
	printf("End Fourier Transform\r\n");
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

	cfg = kiss_fft_alloc(N, 0, NULL, NULL);
  if (cfg != NULL)
  {
    size_t i;

    kiss_fft(cfg, in, out);
    
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
















