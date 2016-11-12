/****************************************************************************
 Module
   ShiftRegisterWrite.c

 Revision
   1.0.1

 Description
   This module acts as the low level interface to a write only shift register.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/21/16 10:46 Max     first pass
 
****************************************************************************/
// C99 types common headers
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// GPIO subsystem headers
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// TivaWare Library headers
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "BITDEFS.H"
#include "ALL_BITS.h"

// readability defines
#define DATA GPIO_PIN_0
#define SCLK GPIO_PIN_1
#define RCLK GPIO_PIN_2

#define DATA_HI BIT0HI
#define DATA_LO BIT0LO
#define SCLK_HI BIT1HI
#define SCLK_LO BIT1LO
#define RCLK_LO BIT2LO
#define RCLK_HI BIT2HI

#define GET_MSB_IN_LSB(x) ((x & 0x80)>>7)

// Retain the last 8 bits written to the shift register
static uint8_t LocalRegisterImage=0;

// CIRC_SHIFT
// Perform a circular shift on a 8 bit integer
uint8_t CIRC_SHIFT(uint8_t v)
{
	uint8_t shift = 1;
	return (v << shift) | (v >> (sizeof(v)*8 - shift));
}

/**********************************************************************
 Function
   SR_INIT
 Parameters
   None
 Returns
   Nothing
 Description
   Initialize the shift register hardware
**********************************************************************/
void SR_Init(void){
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN	| SYSCTL_XTAL_16MHZ);
	
	// Turn on port B and wait a few clock cycles until its on
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
	int Dummy = HWREG(SYSCTL_RCGCGPIO);
	
	// Set port B to digital, and set the direction to out
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (DATA | SCLK | RCLK);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (DATA | SCLK | RCLK);
	
  // start with the data & sclk lines low and the RCLK line high
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) = (DATA_LO & SCLK_LO & RCLK_HI);
}


/**********************************************************************
 Function
   SR_GetCurrentRegister
 Parameters
   None
 Returns
   uint8_t LocalRegisterImage
 Description
   Initialize the shift register hardware
**********************************************************************/
uint8_t SR_GetCurrentRegister(void){
  return LocalRegisterImage;
}


/**********************************************************************
 Function
   SR_Write
 Parameters
   None
 Returns
   uint8_t LocalRegisterImage
 Description
   Write a 8 bit number to the register
**********************************************************************/
void SR_Write(uint8_t NewValue){

	uint8_t NewBit;
  uint8_t BitCounter;
  LocalRegisterImage = NewValue; // save a local copy

	// lower the register clock
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= RCLK_LO;
	
	// shift out the data while pulsing the serial clock  
	for (BitCounter=1; BitCounter<=8; BitCounter++)
	{
		// Isolate the MSB of NewValue, put it into the LSB position and output
		NewValue = CIRC_SHIFT(NewValue);
		NewBit = NewValue & BIT0HI; 
		
		if (NewBit){
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) |= DATA_HI;
		} else {
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= DATA_LO;
		}
		// Debugging message to print binary value
		//printf("\r\nWrote value to shift register: %i",NewBit); 
 		
		// raise shift clock SCsLK
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) |= SCLK_HI;
		
		// lower shift clock SCLK
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) &= SCLK_LO;
	} 
	// raise the register clock to latch the new data
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA+ALL_BITS)) |= RCLK_HI;
	//puts("Wrote to the shift register");
}

