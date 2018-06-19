#ifndef EXCEPTIONS
#define EXCEPTIONS

/********************** EXCEPTIONS.E ****************************
*
*  The externals declaration file for the Exception Handling Module.
*
*  Written by Thomas Reichman, Ajiri Obaebor
*/

#include "../h/types.h"

extern void copyState(state_t* origin, state_t* destination);
extern void loadState();
extern void updateTime();
extern void PGMTrapHandler();
extern void TLBTrapHandler();
extern void SYSCallHandler();

/***************************************************************/

#endif
