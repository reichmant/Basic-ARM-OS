/**************************************************************
* FILENAME:		initial.c
* 
* DESCRIPTION:	Initialization Module for JaeOS
* 
* NOTES:		Sets up all neccessary parts for JaeOS to run.
*				This includes:
*					- Process queues and timing-related globals
*					- Semaphore Array and Device Status Array
*					- 4 main areas in memory
*						SYS, PRGM, TLB, INT
*					- First job to be executed
*
*				For each of the new processor states:
*					PC: address of appropriate handler function
*					SP: RAMTOP - last frame of RAM for its stack
*					CPSR:
*						Mode: kernel (sys)
*						Interruptions: off
*					CP15_Control:
*						Virtual Memory: off
*
*				For the first job:
*					PC: address of 'test' function
*					SP: Penultimate RAM Frame
*					CPSR:
*						Interruptions: enabled
*						Mode: kernel (sys)
*					CP15_Control:
*						Virtual Memory: off
*
* AUTHORS:		Thomas Reichman; Ajiri Obaebor
*				Some descriptions adapted from Michael Goldweber
*				Debugging help from Patrick and Neal
*				C commenting conventions adapted from http://syque.com/cstyle/ch4.htm
**************************************************************/

#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"

#include "../h/const.h"
#include "../h/types.h"
#include "/usr/include/uarm/libuarm.h"

///////////////////////// GLOBAL DEFINITONS //////////////////////////

int 			g_procCount; 			// Number of processes in the system
int 			g_softBlockCount;		// # of processes blocked AND waiting for interrupt

int 			g_startTOD;				// when TOD starts
int 			g_endTOD;				// when TOD ends
int 			g_accTime;				// total time accumulated between above

int 			g_endOfInterval;		// When the interval timer will run dry (calculated "date")

pcb_PTR 		g_currentProc;			// holds the current state that is actually running
pcb_PTR 		g_readyQueue;			// tp->ProcBlk of processes: ready AND waiting for turn of execution

int g_lotOfSemaphores[MAXSEMA4]; 		// array of all semaphores:
								// 8 * (Disks, Tapes, Printers, Networking Devices), 16 Terminals, 1 Clock

int g_deviceStatus[MAXSEMA4]; 			// status held for each device's semaphore

extern void test();

/* ---- main() --------------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		int
* Description:
*	Initialize all OS-wide globals
*	Set up the four "new" areas
*	Create process queues and semaphore lists
*	Allocate a PCB for the first job
*	Set up first job's area and put it on readyQueue
*	Start the PC off in our test() function as first job
*	Calculate when interval will end
*	Call scheduler and never return to this function
* -------------------------------------- end main() ---- */
void main(){
	
	// Set up Global defaults
	g_procCount = 0; 					// no processes yet
	g_softBlockCount = 0; 				// therefore, none blocked and waiting for interrupt

	g_startTOD = 0;						// timer starts at 0
	g_endTOD = 0;						// timer starts at 0
	g_accTime = 0;						// no accumulated time yet
	// end of interval initialized below to account for timing issues
	
	g_currentProc = NULL; 				// none running yet
	g_readyQueue = mkEmptyProcQ(); 		// get an empty queue ready

	// Default all 49 semaphores to 0 (since they're just ints)
	for (int i = 0; i < MAXSEMA4; i++){
		g_lotOfSemaphores[i] = 0;
		g_deviceStatus[i] = 0;
	}
	
	/* //////////// Populate the four New Areas //////////// */
	state_t *procState; 				// set up the state a process could be in

	procState = (state_t *) SYSNEWADD; // change the address to the area we want to initialize
	procState->pc = (unsigned int) SYSCallHandler;		// move PC to the address of appopriate handler function
	procState->sp = RAM_TOP; 				// set stack pointer to last frame of ram
	procState->cpsr = ALLOFF | INTSDISABLED | SYSMODE; // set the modes mentioned above
	procState->CP15_Control = ALLOFF; // VM OFF
	
	// Rest follow the above template
	procState = (state_t *) PGMTNEWADD;
	procState->pc = (unsigned int) PGMTrapHandler;
	procState->sp = RAM_TOP; 				
	procState->cpsr = ALLOFF | INTSDISABLED | SYSMODE;
	procState->CP15_Control = ALLOFF;
								
	procState = (state_t *) TLBNEWADD;
	procState->pc = (unsigned int) TLBTrapHandler;
	procState->sp = RAM_TOP; 				
	procState->cpsr = ALLOFF | INTSDISABLED | SYSMODE;
	procState->CP15_Control = ALLOFF;
	
	procState = (state_t *) INTNEWADD;
	procState->pc = (unsigned int) interruptHandler;
	procState->sp = RAM_TOP; 				
	procState->cpsr = ALLOFF | INTSDISABLED | SYSMODE;
	procState->CP15_Control = ALLOFF; 
	
	/* ///////// Process and Semaphore Setup ///////// */

	initPcbs(); // Initializes the PCBs
	initASL(); // Get ASL ready too
	pcb_PTR firstProc = allocPcb(); // Initalize the very first process
	insertProcQ(&(g_readyQueue), firstProc); // Insert the new process onto ready queue
	// first job is now ready!
	g_procCount = 1;		// we should have exactly 1 process now
	
	/* //////////// Set up first process' state //////////// */

	firstProc->p_s.pc = (unsigned int) test;
	firstProc->p_s.sp = (RAM_TOP - FRAME_SIZE);
	firstProc->p_s.cpsr = ALLOFF | SYSMODE;
	procState->CP15_Control = ALLOFF;
	
	g_endOfInterval = getTODLO() + INTERVAL; // when we know it's the end of our interval
	// DO NOT CHANGE THE LOCATION OF THIS LINE OR WE ARE SCREWED

	setTIMER(QUANTUM); // initialize timer with full quantum
	
	scheduler(); // now let scheduler do the rest of the work
	
	PANIC(); // better not get here!
}
