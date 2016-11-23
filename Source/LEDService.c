/****************************************************************************
 Module
   LEDService.c

 Revision
   1.0.1

 Description
   This is a .c file for implementing lighting RGB LEDs in particular orders
	 
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "ES_DeferRecall.h"
#include "ES_Timers.h"
#include "termio.h"

// the headers to access the GPIO subsystem
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// the headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "ADMulti.h" // ADC Library
#include "PWM10Tiva.h" // PWM Library

// The headers from this project
#include "LEDService.h"
#include "ResetService.h"

#include "BITDEFS.H"

/*----------------------------- Module Defines ----------------------------*/
#define ALL_BITS (0xff<<2)
#define LEDBits 18

#define ONE_SEC 1000
#define ONE_FIFTH_SEC (ONE_SEC/5)
#define ONE_THIRD_SEC (ONE_SEC/3)
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)
#define THREE_SEC (ONE_SEC*3)
#define FIVE_SEC (ONE_SEC*5)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static void SR_Init(void);
static void lightLED(uint32_t LEDHex);
static void lightLEDWelcome(uint32_t LEDHex);
static uint32_t getADCState(void);
static uint32_t getADCDiff(uint32_t CurrentADCState, uint32_t LastADCState);
static uint32_t getRandomNum(void);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static int BitCounter;
static uint32_t LastADCState;
static LEDMode_t CurrentMode;
static uint32_t WelcomeHex;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitLEDService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
****************************************************************************/
bool InitLEDService ( uint8_t Priority )
{
	ES_Event ThisEvent;
	// Initialize the MyPriority variable with the passed in parameter.
  MyPriority = Priority;
	
  // Set LEDBits to 18 (6 RGB LEDs with 3 bits to write) and BitCounter to 0
  BitCounter = 0;
	WelcomeHex = 0x98334000; // A serial data used to light up all LEDs in a welcoming pattern

	// Initialize Shift Register
	SR_Init();
  printf("Shift Register Initialization Completed.\r\n");

  // Initialize ADC
  ADC_MultiInit(4);
	printf("ADC Ready for Scratching Data.\r\n");

	// Initialize timer for future use 
	ES_Timer_Init(ES_Timer_RATE_1mS);
	printf("Timer Initialized.\r\n");

	// Set all bits to 0 initially
	for (int i = 0; i < LEDBits; i++) {
		lightLED(0x00000000);
	}
	printf("LEDs All Clear.\r\n");
	
	// Sample ADC port line PE1 and use it to initialize the LastADCState variable
  LastADCState = getADCState();
  CurrentMode = InitLED;
  
  // Post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      printf("LEDService initialization Completed.\r\n");
			return true;
  } else {
      printf("LEDService initialization Completed.\r\n");
			return false;
  }
}

/****************************************************************************
 Function
     PostLEDService

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise
****************************************************************************/
bool PostLEDService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}


bool CheckLEDEvents(void) {
	//Local ReturnVal = False, CurrentADCState
	bool ReturnVal = false;
	uint32_t CurrentADCState = getADCState();
	ES_Event ThisEvent;
	uint32_t diff = getADCDiff(CurrentADCState, LastADCState);
	//printf("CurrentADCState: %u, LastADCState = %u\r\n", CurrentADCState, LastADCState);
	
	//If the CurrentButtonState is different from the LastButtonState
	if (diff >=100){
		// Tell the reset service that there was interaction
		ThisEvent.EventType = ES_INTERACTION;
		PostResetService(ThisEvent);
		ReturnVal = true;
		
		// If the user selects a particular mode for more than 500ms
		// send that mode to the service. Otherwise we discard the events
		
		// Check if the LEDs are waiting for ADC
		if (CurrentMode == LEDWaiting4ADC){
			double  ADCRange = CurrentADCState / 4096.00;
			if (ADCRange <= 0.10) {
				ThisEvent.EventType = LED_MODE_1;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 01*****/\r\n");
			} else if (ADCRange <= 0.20){ 
				ThisEvent.EventType = LED_MODE_2;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 02*****/\r\n");
			} else if (ADCRange <= 0.30){ 
				ThisEvent.EventType = LED_MODE_3;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 03*****/\r\n");
			} else if (ADCRange <= 0.40){ 
				ThisEvent.EventType = LED_MODE_4;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 04*****/\r\n");
			} else if (ADCRange <= 0.50){ 
				ThisEvent.EventType = LED_MODE_5;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 05*****/\r\n");
			} else if (ADCRange <= 0.60){ 
				ThisEvent.EventType = LED_MODE_6;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 06*****/\r\n");
			} else if (ADCRange <= 0.70){ 
				ThisEvent.EventType = LED_MODE_7;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 07*****/\r\n");
			} else if (ADCRange <= 0.80){ 
				ThisEvent.EventType = LED_MODE_8;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 08*****/\r\n");
			} else if (ADCRange <= 0.90){ 
				ThisEvent.EventType = LED_MODE_9;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 09*****/\r\n");
			} else if (ADCRange <= 1.00){ 
				ThisEvent.EventType = LED_MODE_10;
				PostLEDService(ThisEvent);
				printf("\r/*****LED MODE 10*****/\r\n");
			}
		}
		//Set LastADCState to the CurrentADCState
		LastADCState = CurrentADCState;
	}
	return ReturnVal;
}

/****************************************************************************
 Function
    RunLEDService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise
****************************************************************************/
ES_Event RunLEDService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
	ES_Event TransitionEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
  
  LEDMode_t NextMode = CurrentMode;
  switch (CurrentMode) {
		
		// In this state the LED is ready to rock
		case InitLED:
			if (ThisEvent.EventType == ES_INIT) {
				printf("LEDService Intialized. Starting the welcome performance.\r\n");
				NextMode = LEDWelcomeMode;
				BitCounter = 0;
				WelcomeHex = 0x98334000;
				lightLEDWelcome(WelcomeHex);
			}
		break;

		// In this state the LED is showing the welcome performance
		// When the welcome is finished post the event LIFECYCLE_WELCOME_COMPLETE to lifecycle
		// This automatically mode automatically transitions to the LEDWaitingForADC
	  case LEDWelcomeMode: 
			if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == WELCOME_LED_TIMER) ) { // EventParam is timer index 
				// Recieved a welcome step timout 
				if (BitCounter < LEDBits){
					// Perform another welcome step
					BitCounter++;
					printf("LEDService [Welcome Mode]: Shifted %d times.\r\n", BitCounter);
					WelcomeHex = WelcomeHex << 1;
					lightLEDWelcome(WelcomeHex);
				} else {
					// Welcome mode is complete
					NextMode = LEDWaiting4ADC;
					printf("LEDService [Welcome Mode]: Waiting for ADC data\r\n");
					TransitionEvent.EventType = ES_WELCOME_COMPLETE;
					PostResetService(TransitionEvent);
				}
			}
			break;
		
		// In this state the LED Strip is controlled by the resistor strip
		// This mode can be left by calling LIFECYCLE_RESET_ALL
		case LEDWaiting4ADC:
			if (ThisEvent.EventType == LED_MODE_1) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_2) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_3) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_4) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_5) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_6) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_7) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_8) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_9) lightLED(getRandomNum());
			if (ThisEvent.EventType == LED_MODE_10) lightLED(getRandomNum());	
			if (ThisEvent.EventType == ES_SLEEP) {
				printf("LEDService Sleeping.\r\n");
				lightLED(0x00000000);
				NextMode = Sleeping;
			}
			break;
				
		case Sleeping:
			lightLED(0x00000000); // Do something else
			if (ThisEvent.EventType == ES_WAKE) {
				printf("LED Service goes back to welcome mode.\r\n");
				NextMode = InitLED;
				TransitionEvent.EventType = ES_INIT;
				PostLEDService(TransitionEvent);
			}
			break;
  }
  //Set CurrentMode to NextMode
  CurrentMode = NextMode;
  //Return ES_NO_EVENT
  return ReturnEvent;
}




/***************************************************************************
 private functions
 ***************************************************************************/
void SR_Init(void){
  // Set up port B by enabling the peripheral clock and setting the direction
  // of PB0, PB1 & PB2 to outputs
	HWREG(SYSCTL_RCGCGPIO)|= SYSCTL_RCGCGPIO_R1; 
	while ((HWREG(SYSCTL_RCGCGPIO) & SYSCTL_RCGCGPIO_R1) != SYSCTL_RCGCGPIO_R1);

	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN)|= (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR)|= (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	
  // Start with the Data & SCLK lines (PB0, PB1) low and the RCLK line (PB2) high
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= ~(GPIO_PIN_0 | GPIO_PIN_1);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) |= GPIO_PIN_2;
}

void lightLED(uint32_t LEDHex) {
	// Lower the register clock
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_2);
	// Shift out data while pulsing SCLK
	for(int i=0; i < LEDBits; i++)
	{
		if((LEDHex & 0x80000000) == 0x80000000){
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= GPIO_PIN_0;   
		} else {
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_0);
		}
		
		// Pulse SCLK(PB1)
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= GPIO_PIN_1;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_1);
		LEDHex = LEDHex << 1;
	}
	// Raise the register clock to latch the new data
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (GPIO_PIN_2);
}

void lightLEDWelcome(uint32_t LEDHex) {
	if((LEDHex & 0x80000000) == 0x80000000){
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= GPIO_PIN_0;   
	} else {
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_0);
	}
	
	// Pulse SCLK(PB1)
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= GPIO_PIN_1;
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_1);
	
	// Pulse RCLK(PB2) to latch new data
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_2);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (GPIO_PIN_2);
	
	ES_Timer_InitTimer(WELCOME_LED_TIMER, HALF_SEC); //#define WELCOME_LED_TIMER 4 (timer posts to RunLEDService)
	printf("Welcome Timer Set Up.\r\n");
}

uint32_t getADCState(void) {
	uint32_t ADInput[4];
	uint32_t CurrentInput;
	ADC_MultiRead(ADInput);
	CurrentInput = ADInput[1]; // Get ADC data from PE1
	//printf("CurrentInput is %u.\r\n", CurrentInput);
	return CurrentInput;
}

uint32_t getADCDiff(uint32_t CurrentADCState, uint32_t LastADCState) {
	if (CurrentADCState >= LastADCState) {
		return (CurrentADCState - LastADCState);
	} else {
		return (LastADCState - CurrentADCState);
	}
}

uint32_t getRandomNum(void) {
	uint32_t x = rand() & 0xff;
	x |= (rand() & 0xff) << 8;
	x |= (rand() & 0xff) << 16;
	x |= (rand() & 0xff) << 24;
	return x;
}

