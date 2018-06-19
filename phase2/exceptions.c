/**************************************************************
* FILENAME:		exceptions.c
* 
* DESCRIPTION:	Exception Handling Module for JaeOS
* 
* NOTES:		Take appropriate action for PGM, TLB, and SVC traps
*				SYS calls are handled based on number and user/sys mode,
*				generalizing numbers 9-255 to passUpOrDie().
*				
*				PGM/TLB traps are passed up or killed.
*				SYS calls work in SYS mode,
*				simulate a PGM trap in User mode,
*				and are passed up or killed if SYS 9+
*
*				All SYS calls are handled in their own function,
*				but may call helper functions.
*
*				For more information on a particular SYS call,
*				go to the function-level description for that call.
*
*				This file also contains helper functions used by other files.
*				These include:
*					the ability to overwrite one state with another,
*					a holder for the LDST call on the current process's state,
*					and a function that updates the p_time field of the current process.
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
// Allow old areas to be easily accessed throughout all functions
state_t *oldPGM = (state_t *) PGMTOLDADD;
state_t *oldTLB = (state_t *) TLBOLDADD;
state_t *oldSYS = (state_t *) SYSOLDADD;

////////////////////// TABLE OF CONTENTS //////////////////////
/********************* Public Functions **********************/
//	   void copyState(state_t* origin, state_t* destination);
//	   void loadState();
//	   void updateTime();
//	   void PGMTrapHandler();
//	   void TLBTrapHandler();
//	   void SYSCallHandler();
/********************* Private Functions *********************/
HIDDEN void createProcess ();
HIDDEN void terminateProcess ();
HIDDEN void verhogen ();
HIDDEN void passeren ();
HIDDEN void spectrapvec ();
HIDDEN void getCPUTime ();
HIDDEN void waitClock ();
HIDDEN void waitIO ();
HIDDEN void depthFirstMurder (pcb_PTR observedProcess);
HIDDEN void passUpOrDie (int trapType, state_t *oldState);
//////////////////// END TABLE OF CONTENTS ////////////////////


/* ---- copyState() ---------------------------------------
* Parameters: 	state to be copied and state it will override
* Type: 		Public
* Return:		None
* Description:
*	Overwrites the destination's registers with those in the origin's
*		This applies to ALL registers.
* --------------------------------- end copyState() ---- */
void copyState(state_t* origin, state_t* destination){
	destination->a1 = origin->a1;
	destination->a2 = origin->a2;
	destination->a3 = origin->a3;
	destination->a4 = origin->a4;
	destination->v1 = origin->v1;
	destination->v2 = origin->v2;
	destination->v3 = origin->v3;
	destination->v4 = origin->v4;
	destination->v5 = origin->v5;
	destination->v6 = origin->v6;
	destination->sl = origin->sl;
	destination->fp = origin->fp;
	destination->ip = origin->ip;
	destination->sp = origin->sp;
	destination->lr = origin->lr;
	destination->pc = origin->pc;
	destination->cpsr = origin->cpsr;
	destination->CP15_Control = origin->CP15_Control;
	destination->CP15_EntryHi = origin->CP15_EntryHi;
	destination->CP15_Cause = origin->CP15_Cause;
	destination->TOD_Hi = origin->TOD_Hi;
	destination->TOD_Low = origin->TOD_Low;
}

/* ---- loadState() --------------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None (ends up in PC from currentProc's state)
* Description:
*	Container for the LDST function
*	It is always assumed that we are loading the one in currentProc
*	If we ever want to have the ability to load something else,
*		a parameter option will be implemented.
*	(But so far, we've only wanted to load this state)
*	Just don't call it when currentProc is NULL!
* -------------------------------------- end loadState() ---- */
void loadState(){
	LDST(&(g_currentProc->p_s));
}

/* ---- updateTime() --------------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None
* Description:
*	Update the p_time field of the current process
*		and the amount of time remaining.
* -------------------------------------- end updateTime() ---- */
void updateTime(){
	g_endTOD = getTODLO(); 						// endpoint created so we can figure out time difference
	g_accTime = g_endTOD - g_startTOD; 			// Calculate how much time has passed since the start
	g_currentProc->p_time = g_currentProc->p_time + g_accTime; // Update that for the current process

	/* Move start time so that future calculations don't charge for
	   time already charged to them while still charging them for their time
	   in here 															*/
	g_startTOD = g_endTOD;
}

/* ---- PGMTrapHandler() ---------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None
* Description:
*	Just pass up if possible, kill otherwise.
*	It simply gives passUpOrDie() the necessary parameters.
* --------------------------------- end PGMTrapHandler() ---- */
void PGMTrapHandler(){
	passUpOrDie(PGMTRAP, oldPGM);
	
}

/* ---- TLBTrapHandler() ---------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None
* Description:
*	Just pass up if possible, kill otherwise.
*	It simply gives passUpOrDie() the necessary parameters.
* --------------------------------- end TLBTrapHandler() ---- */
void TLBTrapHandler(){
	passUpOrDie(TLBTRAP, oldTLB);
	
}

/* ---- SYSCallHandler() ---------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None
* Description:
*	There exists 3 potential cases to be handled:
*		SYS call 9-255: passUpOrDie()
*		SYS call 1-8 in SYS mode: Handled individually
*		SYS call 1-8 NOT in SYS mode: Simulate PGMTrap
* --------------------------------- end SYSCallHandler() ---- */
void SYSCallHandler(){
	copyState(oldSYS, &(g_currentProc->p_s)); // current process' state is
												// now what was stored in oldSYS
	int SYSNum = oldSYS->a1; // Extract SYS # from A1

	// CASE 1: SYS call number is NOT one of the ones we can handle
	if(SYSNum > WAITIO){
		passUpOrDie(SYSTRAP, oldSYS);
	}
	
	// CASE 2: We are in SYS mode
	if((g_currentProc->p_s.cpsr & SYSMODE) == SYSMODE){

		switch(SYSNum){		// Handle each SYS num individually
			case CREATEPROCESS:
				createProcess((state_t *) oldSYS->a2);
				break;

			case TERMINATEPROCESS:
				terminateProcess();
				break;
				
			case VERHOGEN:
				verhogen((int *) oldSYS->a2);
				break;

			case PASSEREN:
				passeren((int *) oldSYS->a2);
				break;
			
			case SPECTRAPVEC:
				spectrapvec((int *) oldSYS->a2, (state_t *) oldSYS->a3, (state_t *) oldSYS->a4);
				break;
			
			case GETCPUTIME:
				getCPUTime();
				break;
				
			case WAITCLOCK:
				waitClock();
				break;
				
			case WAITIO:
				waitIO((int *) oldSYS->a2, (state_t *) oldSYS->a3, (state_t *) oldSYS->a4);
				break;
		}
	}
	
	// CASE2: We are NOT in SYS mode - simulate a program trap
		// JaeOS.pdf page 23:
		// "The above eight nucleus services are considered privileged services and are only
		// available to processes executing in kernel-mode. Any attempt to request one of
		// these services while in user-mode should trigger a PgmTrap exception response.
	
	copyState(oldSYS, oldPGM);		// This is done by moving the processor state from
									// the SVC/BKPT Old Area to the PgmTrap Old Area,

	oldPGM->CP15_Cause = RI; 		// setting CP15.Cause in the PgmTrap Old Area to RI,

	PGMTrapHandler(); 				// and calling JaeOS’s Pgm-Trap exception handler."
	}
	

/* ---- createProcess() --------------------------------------------
* Parameters: 	Physical address of a processor state (from A2)
* Type: 		Private
* Return:		Success/Failure state in A1
* Description:	SYS 1
*	Make a new process to be put on the ready queue.
*	It is a child of the process calling SYS 1 (g_currentProc)
*	The child inherits the state of the parent.
*	A1 contains the success/failure state of the operation
* -------------------------------------- end createProcess() ---- */
HIDDEN void createProcess(state_t *parentState){
	pcb_PTR newPcb = allocPcb(); // Get a pcb ready for the new process
	
	if(newPcb != NULL){ // make sure we actually got something
		copyState(parentState, &(newPcb->p_s)); // inherit parent's state
		
		insertChild(g_currentProc, newPcb); // new proc is child of current proc (the parent)
		insertProcQ(&(g_readyQueue), newPcb); // and now it's on the g_readyQueue
		g_procCount++; 					// hooray, new process!

		parentState->a1 = SUCCESS; 	// Success Flag
	}
	
	else{ // if we didn't get a pcb, we failed!
		parentState->a1 = FAILURE; 	// Failure Flage
	}
				
	loadState(); // go back to where we left off
}

/* ---- terminateProcess() --------------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		None
* Description:	SYS 2
*	Kill a process
*		 And its children
*		   And its children's children (etc.)
*	Then, get a new job.
* -------------------------------------- end terminateProcess() ---- */
HIDDEN void terminateProcess(){
	depthFirstMurder(g_currentProc); 	// Hooray, recursion!
	// now nothing is current process, so...
	scheduler(); 	// BRING ME ANOTHER
}

/* ---- verhogen() --------------------------------------------
* Parameters: 	Physical address of the semaphore to be V'ed (from A2)
* Type: 		Private
* Return:		None
* Description:	SYS 3
*	Good ol' V operation (signaling)
*	Assert mutex and unblock a process, put on g_readyQueue
*	Return to current process
* -------------------------------------- end verhogen() ---- */
HIDDEN void verhogen(int *semAdd){
	(*semAdd)++; // increment the semaphore of the one to be V'ed
	
	if(*semAdd <= 0){
		
		pcb_PTR signaledProc = removeBlocked(semAdd); // pop it off
		if (signaledProc == NULL) { // will we always get something?
			PANIC(); // not sure what we'd do if we don't...
			// I mean, we'd probably PANIC(); even without this if statement
		}
		signaledProc->p_semAdd = NULL;
		
		insertProcQ(&(g_readyQueue), signaledProc); // put the signaled one on the readyQueue
	}

	loadState(); // go back to where we left off
}

/* ---- passeren() --------------------------------------------
* Parameters: 	Physical address of the semaphore to be P'ed (from a2)
* Type: 		Private
* Return:		None
* Description:	SYS 4
*	P operation (waiting)
* -------------------------------------- end passeren() ---- */	
HIDDEN void passeren(int *semAdd){
	(*semAdd)--; // decrement the semaphore address of the one to be P'ed
									
	if(*semAdd < 0){

		updateTime(); // Update the time used by this process

		insertBlocked(semAdd, g_currentProc); // block current process

		g_currentProc = NULL; // done with the current process
		scheduler(); // so we need someone else
	}

	loadState(); // go back to where we left off
}

/* ---- spectrapvec() --------------------------------------------
* Parameters: 	Trap type (integer "exception code");
*				The addresses into which the OLD and NEW processor 
* 				states are to be stored when an exception occurs
*				while running this process.
* Type: 		Private
* Return:		None
* Description:	SYS 5
*	Save the contents of A3 and A4 (in the invoking procs’ ProcBlk)
*	to facilitate “passing up” handling of the respective exception.
*
*	When an exception occurs for which an Exception State Vector has been specified,
*	the nucleus stores the processor state in the area pointed to by the address in A3
*	and loads the new processor state from the area pointed to by the address given in A4.
*
*	Each process may request a SVC 5 service at most once for each of the three
*	exception types. An attempt to request a SVC 5 service more than once per exception
*	type OR exception without specified Exception State Vector will simulate a SVC 2 service.
* -------------------------------------- end spectrapvec() ---- */	
HIDDEN void spectrapvec(int trapType, state_t *oldProcState, state_t *newProcState){
	// Case 1: SYS 5 wasn't called more than once for this trap type
	if(g_currentProc->stateArray[trapType].oldState == NULL){ 
		g_currentProc->stateArray[trapType].oldState = oldProcState;
		g_currentProc->stateArray[trapType].newState = newProcState;
		
		loadState();
	}

	// Case 2: SYS 5 was called more than once for this trap type
	terminateProcess(); // Simulate SYS 2
}

/* ---- getCPUTime() --------------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		Current process time (in A1)
* Description:	SYS 6
*	Puts the processor time (in microseconds) used by the requesting
*	process to be placed in the caller’s A1.
*
*	A process is charged for time spent in this function
* -------------------------------------- end getCPUTime() ---- */	
HIDDEN void getCPUTime(){
	updateTime(); // Update the time used by this processor since start

	// Write current process time into A1 to be returned
	g_currentProc->p_s.a1 = g_currentProc->p_time;
		
	loadState();
}

/* ---- waitClock() --------------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		None
* Description:	SYS 7
*	Does a P operation on semaphore that nucleus maintains for
*	the pseudo-clock timer.
*	The current process is now waiting for the clock
*	and the scheduler is called.
* -------------------------------------- end waitClock() ---- */	
HIDDEN void waitClock(){
	// Standard P operation, where the clock device's semaphore is located at the last index
	g_lotOfSemaphores[CLOCKINDEX] = g_lotOfSemaphores[CLOCKINDEX] - 1; // Decrement semaphore address
	
	if(g_lotOfSemaphores[CLOCKINDEX] < 0){ // This should be true...
		
		updateTime(); // Update the time used by this process
		
		// Current proc blocked off and waiting on clock
		insertBlocked(&(g_lotOfSemaphores[CLOCKINDEX]), g_currentProc); 
		g_softBlockCount++; // since we blocked something waiting for interrupt

		g_currentProc = NULL; // done with the current process				
		scheduler();
	}

	PANIC(); // Should never NOT be able to wait for the clock (can't increment semaphore)
}

/* ---- waitIO() --------------------------------------------
* Parameters: 	interrupt line number (A2),
*				device number (A3),
* 				whether or not to wait for terminal read (A4)
* Type: 		Private
* Return:		None
* Description:	SYS 8
*	Does a P operation on semaphore that nucleus maintains for
*	I/O device specified by A2, A3, and possibly A4.
*	The current process is now waiting for this device
*	and the scheduler is called.
* -------------------------------------- end waitIO() ---- */
HIDDEN void waitIO(int intlNO, int dnum, BOOL waitForTermRead){
	// Get the index used to locate the sempahore address of our device
	int semaphoreIndex = getSemaphoreIndex(intlNO, dnum);

	if((intlNO == LINENUMSEVEN) && (waitForTermRead == FALSE)){ // If its a transmitting terminal
		semaphoreIndex = semaphoreIndex + TOTALDEVICES; // increment to this set of subdevices
	}
	
	// P operation stuff
	g_lotOfSemaphores[semaphoreIndex] = g_lotOfSemaphores[semaphoreIndex] - 1; // Decrement semaphore

	// Case 1: The interrupt DID NOT occur prior to SYS 8 Call.
	if(g_lotOfSemaphores[semaphoreIndex] < 0){
							
		updateTime();

		// Current proc blocked off and waiting on that device
		insertBlocked(&(g_lotOfSemaphores[semaphoreIndex]), g_currentProc);
		g_softBlockCount++; // since we blocked something waiting for interrupt
		
		g_currentProc = NULL; // done with the current process
		scheduler();
	}

	/* Case 2: The interrupt occured prior to SYS 8 Call.
	     Since current proc not blocked as a result of the P operation,
	     interrupting device’s status word placed in A1 prior to resuming execution. */
	g_currentProc->p_s.a1 = g_deviceStatus[semaphoreIndex];
	loadState();
}


/* ---- depthFirstMurder() --------------------------------------------
* Parameters: 	pcb_PTR observedProcess
* Type: 		Private
* Return:		None (but scheduler should be called afterwards)
* Description:
*	Kill a process, it's children, and so on. 	
*	Has cases for if the process killed was:
*		1: the current process
*		2: on the ready queue
*		3: blocked by a semaphore
*	This will usually be called on currentProc (since it can only be accessed
*	by a SYS mode-priveleged call), but it is not hardcoded and accounts
*	for the currentProc scenario in a modular fashion.
* -------------------------------------- end depthFirstMurder() ---- */
HIDDEN void depthFirstMurder(pcb_PTR observedProcess){	
	while(!emptyChild(observedProcess)){ // Depth-first search for the "bottom" child
		depthFirstMurder(removeChild(observedProcess)); // Kill each child before their parent
	}
	
	// We've reached this point once we've killed all of observedProcess's children (or observedProcess never had children)

	// Case 1: observedProcess is current process
	if (g_currentProc == observedProcess){
		outChild(observedProcess); // no longer anyone's child
		g_currentProc = NULL;
	}
	
	// Case 2: observedProcess is on the readyQueue
	else if(observedProcess->p_semAdd == NULL){
		outProcQ(&(g_readyQueue), observedProcess); // Taken off readyQueue since you're dead
	}
	
	// Case 3: observedProcess is on the ASL
	else{
		outBlocked(observedProcess);
		
		// Check if the semAdd refers to a device - we have 49, so it must be between 0 and 48
		if((observedProcess->p_semAdd >= &(g_lotOfSemaphores[0])) && (observedProcess->p_semAdd <= &(g_lotOfSemaphores[LASTSEMINDEX]))){
			g_softBlockCount--; // One less proc waiting on them
		}
		
		else{
			*(observedProcess->p_semAdd) = *(observedProcess->p_semAdd) + 1; // Increment semaphore because one less waiting
		}
	}
	
	freePcb(observedProcess); // Finally, we can kill this node for good
	g_procCount--; // Which means one less process!
}

/* ---- passUpOrDie() --------------------------------------------
* Parameters: 	int trapType, state_t *oldState
* Type: 		Private
* Return:		None
* Description:
*	If a process has handler set up, pass it up appropriately;
*		return to previous position.
*	Otherwise, kill it and its children recursively.
*
*	In other words,
*		If the offending process has NOT issued a SVC 5 for SVC/BKPT
*			exceptions, simulate SYS 2.
*		Otherwise, “pass it up.”
*			The processor state is moved from the SVC/BKPT Old Area into
*			the respective ProcBlk's Old Area Address.
*			Finally, the state whose address was recorded in the ProcBlk as
*			the respective New Area Address is made the current processor state.
* -------------------------------------- end passUpOrDie() ---- */
HIDDEN void passUpOrDie(int trapType, state_t *oldState){
	// Case 1: SYS 5 had not been called
	if(g_currentProc->stateArray[trapType].newState == NULL) { 
		terminateProcess();
	} 

	// Case 2: SYS 5 was called - Pass up appropriately
	copyState(oldState, g_currentProc->stateArray[trapType].oldState);
	copyState(g_currentProc->stateArray[trapType].newState, &(g_currentProc->p_s)); 
	loadState(&(g_currentProc->p_s));
}
