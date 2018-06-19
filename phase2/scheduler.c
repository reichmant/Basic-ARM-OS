/**************************************************************
* FILENAME:		scheduler.c
* 
* DESCRIPTION:	Round-Robin Process Scheduling Module for JaeOS
* 
* NOTES:		
*				Asserts that all ready jobs are ran and guarantees finite progress.
*
*				Assumes there is no current process at the time - this should be done
*					prior to calling the scheduler.
*
*				Performs simple deadlock detection:
*			
*				If the Ready Queue is empty:
*					1. If the Process Count is zero invoke the HALT ROM service/instruction.
*					2. Deadlock for JaeOS is defined as when the Process Count > 0 and the Softblock
*						Count is zero. Take an appropriate deadlock detected action.
*						(e.g. Invoke the PANIC ROM service/instruction.)
*					3. If the Process Count > 0 and the Soft-block Count > 0 enter a Wait State.
*			
*				If it is in neither an error, complete, nor wait state,
*					1. Put time on the interval timer.
*					2. Record time on the time of day clock
*						(before calling load state on the state on the PCB)
*					3. Store it in the global variable for startTOD
*
* AUTHORS:		Thomas Reichman; Ajiri Obaebor
*				Some descriptions adapted from Michael Goldweber
*				Debugging help from Patrick and Neal
*				Interval timer concept adapted from Neal/Timmy
*				C commenting conventions adapted from http://syque.com/cstyle/ch4.htm
**************************************************************/

#include "../e/pcb.e"
#include "../e/initial.e"
#include "../e/scheduler.e"
#include "../e/exceptions.e"

#include "../h/const.h"
#include "../h/types.h"
#include "/usr/include/uarm/libuarm.h"

void scheduler(){
	// Case 1: We are in an error, complete, or wait state
	// 	(Follows the "tree" above)
	if (emptyProcQ(g_readyQueue)){
		if(g_procCount == 0){		// done with all jobs
			HALT();
		}	
		if(g_softBlockCount == 0){	// deadlock acheived
			PANIC();
		}

		setSTATUS(ALLOFF & INTSENABLED | SYSMODE); 	// modify existing status
		
		g_endOfInterval = getTODLO() + INTERVAL;	// update when the interval should end
		setTIMER(g_endOfInterval - getTODLO()); 	// wait for remainder of timer
		WAIT();
	}

	// Case 2: No errors or waiting. Time to set up a new process.

	g_currentProc = removeProcQ(&(g_readyQueue));

		// Case 2a: You don't have a partial quantum left
	if( (g_endOfInterval - getTODLO()) < 0 || (g_endOfInterval - getTODLO()) >= QUANTUM){
		setTIMER(QUANTUM); // You poor thing... This one's on the house!
	}
		// Case 2b: You have a partial quantum
	else{
		setTIMER(g_endOfInterval-getTODLO());	// Sorry, no refills
	}
	
	g_startTOD = getTODLO(); 					// Start timer before heading off
	loadState();
	
}