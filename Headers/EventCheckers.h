/****************************************************************************
 Module
     EventCheckers.h
 Description
     header file for the event checking functions

*****************************************************************************/

#ifndef EventCheckers_H
#define EventCheckers_H

// the common headers for C99 types 
#include <stdint.h>
#include <stdbool.h>

// prototypes for event checkers

bool Check4Keystroke(void);
bool Check4LEDService(void);
bool Check4Knob(void);
bool Check4ResetButton(void);

#endif /* EventCheckers_H */
