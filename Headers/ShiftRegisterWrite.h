/********************************************************************

  Header file ShiftRegisterWrite

 *********************************************************************/

#ifndef SHIFT_REGISTER_WRITE
#define SHIFT_REGISTER_WRITE

#include "ES_Types.h"
void SR_Init(void);
uint8_t SR_GetCurrentRegister(void);
void SR_Write(uint8_t NewValue);
uint8_t CIRC_SHIFT(uint8_t v);

#endif /* SHIFT_REGISTER_WRITE */
