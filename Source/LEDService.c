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
#include <stdio.h>

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
#define QUARTER_SEC (ONE_SEC/4)
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
static uint32_t getRandomNum(void);  // Used for testing LEDs, but not used in final demo
static void LEDSleepingSequence(void);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static int BitCounter;
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
	WelcomeHex = 0xD5BB8000; // A serial data used to light up all LEDs in a welcoming pattern

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
				WelcomeHex = 0xD5BB8000;
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
			if (ThisEvent.EventType==RESISTIVE_STRIP_CHANGED){
				printf("LED Moving to Mode %i\r\n\n",ThisEvent.EventParam);
				if (ThisEvent.EventParam == 0) {
					/*lightLED(0x88950000);*/ 
					//for (int i=0; i<18; i++) {
					printf("The hex num in binary: 10001000100101010000000000000000\r\n");
					lightLED(0x88950000);
					//}
				}
				
				if (ThisEvent.EventParam == 0) lightLED(0x88950000);
				if (ThisEvent.EventParam == 1) lightLED(0x47628000);
				if (ThisEvent.EventParam == 2) lightLED(0x39BC4000);
				if (ThisEvent.EventParam == 3) lightLED(0xCE478000);
				if (ThisEvent.EventParam == 4) lightLED(0x7128C000);
				if (ThisEvent.EventParam == 5) lightLED(0x8AD50000);
				if (ThisEvent.EventParam == 6) lightLED(0x57FA8000);
				if (ThisEvent.EventParam == 7) lightLED(0xBD2F4000); //Changed to make symmetric 11/29/16
				if (ThisEvent.EventParam == 8) lightLED(0xF2D9C000); //Changed to make symmetric 11/29/16
				if (ThisEvent.EventParam == 9) lightLED(0x4C968000); //Changed to make symmetric 11/29/16
			}
			
			if (ThisEvent.EventType == ES_SLEEP) {
				printf("LEDService Sleeping.\r\n");
				//for (int i=0; i<18; i++) {
					LEDSleepingSequence();
				//}
				NextMode = Sleeping;
			}
			break;
				
		case Sleeping:
			if (ThisEvent.EventType == ES_WAKE) {
				printf("LED Service goes back to welcome mode.\r\n");
				NextMode = InitLED;
				//for (int i=0; i <18; i++) {
					lightLED(0x00000000); // Clear all LED bits for a new round
				//}
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


/****************************************************************************
 Function
    SR_Init

 Parameters
   None

 Returns
   Nothing

 Description
   Initialize the shift register
****************************************************************************/
static void SR_Init(void){
  // Set up port B by enabling the peripheral clock and setting the direction
  // of PB0, PB1 & PB2 to outputs
	HWREG(SYSCTL_RCGCGPIO)|= SYSCTL_RCGCGPIO_R1; 
	while ((HWREG(SYSCTL_RCGCGPIO) & SYSCTL_RCGCGPIO_R1) != SYSCTL_RCGCGPIO_R1);

	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN)|= (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR)|= (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	
  // Start with the Data & SCLK lines (PB0, PB1) low and the RCLK line (PB2)low
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= ~(GPIO_PIN_0 | GPIO_PIN_1);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= ~GPIO_PIN_2;
}



/****************************************************************************
 Function
    lightLED

 Parameters
   uint32_t LEDHex

 Returns
   Nothing

 Description
   Light the LED in a pattern corrosponding to LEDHex
****************************************************************************/
void lightLED(uint32_t LEDHex) {
	// Lower the register clock
	//HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_2);
	// Shift out data while pulsing SCLK
	
	printf("\r\n");
	for(int i=0; i < LEDBits; i++)
	{
		if((LEDHex & 0x80000000) == 0x80000000){
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= GPIO_PIN_0;   
			printf("1");
		} else {
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_0);
			printf("0");
		}
		
		// Pulse SCLK(PB1)
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= GPIO_PIN_1;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_1);
		LEDHex = LEDHex << 1;
	}
	printf("\r\n");
	// Raise the register clock to latch the new data
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (GPIO_PIN_2);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_2);
}


/****************************************************************************
 Function
    lightLEDWelcome

 Parameters
   uint32_t LEDHex

 Returns
   Nothing

 Description
   Display the welcome performance
****************************************************************************/
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
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (GPIO_PIN_2);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_2);
	
	
	ES_Timer_InitTimer(WELCOME_LED_TIMER, QUARTER_SEC); //(timer posts to RunLEDService)
	//printf("Welcome Timer Set Up.\r\n");
}


/****************************************************************************
 Function
    getRandomNum
 Parameters
   None
 Returns
   uint32_t RandonNum. A Random uint32 number
****************************************************************************/
uint32_t getRandomNum(void) {
	uint32_t x = rand() & 0xff;
	x |= (rand() & 0xff) << 8;
	x |= (rand() & 0xff) << 16;
	x |= (rand() & 0xff) << 24;
	return x;
}

/****************************************************************************
 Function
    LEDSleepingSequence
 Parameters
   None
 Returns
   None
 Description
   Display the sleeping performance
****************************************************************************/
void LEDSleepingSequence(void) {
	lightLED(0xFFFFC000);
//	lightLED(0x9FFF4000);
//	lightLED(0xD3FA4000);
//	lightLED(0x5A52C000);
//	lightLED(0x5BF2C000);
//	lightLED(0x5FFEC000);
//	lightLED(0xFFFFC000);
//	lightLED(0xFD3FC000);
//	lightLED(0xEB17C000);
//	lightLED(0x5A52C000);
//	lightLED(0xFA53C000);
//	lightLED(0xFE5FC000);
//	lightLED(0xFFFFC000);	
}
