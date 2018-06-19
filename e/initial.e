#ifndef INITIAL
#define INITIAL

/************************* INITIAL.E *****************************
*
*  The externals declaration file for the Initialization
*    Module.
*
*  Written by Thomas Reichman, Ajiri Obaebor
*/

#include "../h/const.h"
#include "../h/types.h"

extern int 			g_procCount; 			// Number of processes in the system
extern int 			g_softBlockCount;		// # of processes blocked AND waiting for interrupt

extern int 			g_startTOD;				// when TOD starts
extern int 			g_endTOD;				// when TOD ends
extern int 			g_accTime;				// total time accumulated between above

extern int 			g_endOfInterval;		// When the interval timer will run dry (calculated "date")

extern pcb_PTR 		g_currentProc;			// holds the current state that is actually running
extern pcb_PTR 		g_readyQueue;			// tp->ProcBlk of processes: ready AND waiting for turn of execution

extern int g_lotOfSemaphores[MAXSEMA4]; 	// initialize all semaphores:
								// 8 * (Disks, Tapes, Printers, Networking Devices), 16 Terminals, 1 Clock

extern int g_deviceStatus[MAXSEMA4]; 		// status held for each device's semaphore

extern void main ();

/***************************************************************/

#endif