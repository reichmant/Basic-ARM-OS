#ifndef INTERRUPTS
#define INTERRUPTS

/********************** ITERRUPTS.E ****************************
*
*  The externals declaration file for the Interrupt Handling Module.
*
*  Written by Thomas Reichman, Ajiri Obaebor
*/

#include "../h/types.h"

extern void interruptHandler();
extern int getSemaphoreIndex(int trueLineNumber, int devNum);
/***************************************************************/

#endif
