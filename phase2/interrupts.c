/**************************************************************
* FILENAME:		interrupts.c
* 
* DESCRIPTION:	Interrupt Handling Module for JaeOS
* 
* NOTES:		Handles the highest priority interrupting line number.
*				Line 0, 1 interrupts are unsupported
*				Line 2 interrupts are handled based on whether it
*					was the interval timer or end of a quantum.
*				Line 3-6 interrupts are handled with a V operation and ACK.
*				Line 7 interrupts are handled the same as above,
*					but account for the subdevices.
*					Transmitting terminals have higher priority.
*
*				Device interrupts are handled Round-Robin.
*				Multiple interrupts may be handled, but only one at a time.
*				Priority begins on line 0, with line 7 being the lowest.
*				The line number is derived from the CP15 Cause Register.
*
* AUTHORS:		Thomas Reichman; Ajiri Obaebor
*				Some descriptions adapted from Michael Goldweber
*				Debugging help from Patrick and Neal
*				Interval timer handler implementation by Neal/Patrick
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

////////////////////// TABLE OF CONTENTS //////////////////////
/********************* Public Functions **********************/
//	   void interruptHandler();
//	   int getSemaphoreIndex(int trueLineNumber, int devNum);
/********************* Private Functions *********************/
HIDDEN int getLineNumber(state_t *oldINT);
HIDDEN void lineTwoHandler();
HIDDEN void intervalTimerHandler();
HIDDEN void endOfQuantum();
HIDDEN int getDeviceNumber(unsigned int *pendingIntMap);
HIDDEN void externalDeviceHandler(int semaphoreIndex, int trueLineNumber);
//////////////////// END TABLE OF CONTENTS ////////////////////


/* ---- interruptHandler() ---------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None
* Description:
*	There exists 3 scenarios to handle:
*		1. A multiprocessor interrupt (line 0 or 1: unsupported)
*		2. An internally generated interrupt (line 2: handled individually)
*		3. An external device interrupt (lines 3-7: handled generically)
*			(V operation, shut off alarm)
*			Note that line 7 is handled slightly differently
*		We also PANIC if no interrupt was on.
*	See page 35 in the Principles of Operation text for more information
* --------------------------------- end interruptHandler() ---- */
void interruptHandler(){
	state_t* oldINT = (state_t *) INTOLDADD;  // ready the oldINT state
	int trueLineNumber = getLineNumber(oldINT); // extract the trueLineNumber from it
	// (this is a simplified 0-7 integer)

	oldINT->pc = oldINT->pc - PCPREFETCH; // Decrement the PC to compensate prefetching
	
	// If something was running, update time and state
	if(g_currentProc != NULL){
		updateTime();
		copyState(oldINT, &(g_currentProc->p_s));
	}
	
	// The Pending Interrupts Bitmap contains the bits neccessary to determine
	//	the device number. It is different for each device.
	// Only lines 3-7 correspond with external device, so we default to NULL.
	unsigned int *pendingIntMap = NULL;

	// We will now use the simplified line number to determine where the
	//	appropriate Pending Interrupts Bitmap is located.
	// If we are in a special scenario (line 0-2 interrupt),
	//	it is handled independently since it does not refer to an external device.
	switch (trueLineNumber){
		case LINENUMTWO:
			lineTwoHandler();
			break;
	
		case LINENUMTHREE:
			pendingIntMap = (unsigned int *) DISKINTMAP;
			break;
			
		case LINENUMFOUR:
			pendingIntMap = (unsigned int *) TAPEINTMAP;
			break;
			
		case LINENUMFIVE:
			pendingIntMap = (unsigned int *) NETWORKINTMAP;
			break;
			
		case LINENUMSIX:
			pendingIntMap = (unsigned int *) PRINTERINTMAP;
			break;
			
		case LINENUMSEVEN:
			pendingIntMap = (unsigned int *) TERMINALINTMAP;
			break;
		
		default: // Handle line 0 or 1 interrupt
			PANIC(); // (we PANIC during getLineNumber() if no interrupt was on)
			break;
	}

	// The device number and index must be calculated for the V operation to
	//	be performed successfully.
	int deviceNumber = getDeviceNumber(pendingIntMap);
	int semaphoreIndex = getSemaphoreIndex(trueLineNumber, deviceNumber);

	externalDeviceHandler(semaphoreIndex, trueLineNumber);
}

/* ---- getLineNumber() ---------------------------------------
* Parameters: 	oldINT state
* Type: 		Private
* Return:		The actual line number (int)
* Description:
*	1. Get the last 8 bits from the cause register
*	2. Iteratively bit-wise & these bits with the possible line values in order
*		(based on priority)
*	2a.	If the line number matches, return the "true" value (a simple 0-7)
*	2b. If none of the lines were on, PANIC!
* --------------------------------- end getLineNumber() ---- */
HIDDEN int getLineNumber(state_t *oldINT){
	// 1. Get the portion of the cause register which represents which line numbers are turned on
	int truncatedLineNumber = oldINT->CP15_Cause >> LINENUMOFFSET; 

	// 2. Traverse an array of potential line numbers and perform a bitwise & with that.
	// 		These are just hexadecimal equivalents of every possible
	// 		bit position being enabled independently.
	int bitArray[] = {LINEZERO, LINEONE, LINETWO, LINETHREE, LINEFOUR, LINEFIVE, LINESIX, LINESEVEN};
	
	// Check if line number corresponds, starting with highest priority, etc.
	for(int i = 0; i < TOTALLINENUMS; i++)
	{
		if((truncatedLineNumber & (bitArray[i])) != 0) // & with each one, return the one that matches!
		{
			return i; 		// 2a. Found it!
		}
	}

	PANIC(); // 2b. no interrupt was on...
	// or should we handle this differently? maybe ignore? or return?
	// I mean, the interrupt handler should only be called if necessary...
}

/* ---- lineTwoHandler() ---------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		None
* Description:
*	On a line 2 interrupt, there are two possible scenarios:
*		1. It was the interval timer
*		2. It was the end of a quantum
*	These are handled accordingly.
* --------------------------------- end lineTwoHandler() ---- */
HIDDEN void lineTwoHandler(){
	
	// Case 1: Interval Timer passed
	if (getTODLO() >= g_endOfInterval){
		intervalTimerHandler();
	}

	// Case 2: Just the end of a quantum
	endOfQuantum();

}

/* ---- intervalTimerHandler() ---------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		None
* Description:
*	Wake everyone up who was SYS 7 (waiting on interval timer)
*	Refill quantum/interval timers
*	Restart the clock
*	Return if someone was running, else get someone new
* --------------------------------- end intervalTimerHandler() ---- */
HIDDEN void intervalTimerHandler(){
	// unblock one
	pcb_PTR observedProcess = removeBlocked(&(g_lotOfSemaphores[CLOCKINDEX])); // #6, 7
	
	// keep unblocking and adding to ready queue
	while(observedProcess != NULL){ // until no one is left on the ASL
		
		observedProcess->p_semAdd = NULL; // nullify semAdd
		insertProcQ(&(g_readyQueue), observedProcess); // put on g_readyQueue #9

		g_softBlockCount--; // update softBlockCount

		observedProcess = removeBlocked(&(g_lotOfSemaphores[CLOCKINDEX]));
		
	}

	g_lotOfSemaphores[CLOCKINDEX] = 0; // reset clock semaphore - no one is left
					// that's why we broke the loop - no one waiting on it anymore

	// Prepare for next call to schedule
	setTIMER(QUANTUM); //reset quantum timer

	g_endOfInterval = getTODLO() + INTERVAL; // reset interval timer
					
	// Case 1: Someone was running when the interrupt was called
	if(g_currentProc != NULL){
		g_startTOD = getTODLO(); // If so, start the clock
		loadState(); // And load its state
	}
	
	// Case 2: No one running.
	scheduler(); // Need to get a new job.
}

/* ---- endOfQuantum() ---------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		None
* Description:
*	When a process is out of time, it's moved to the end of the g_readyQueue.
*		(since it's still ready - just out of time)
*	We need to refill the timer to a full quantum as well.
* --------------------------------- end endOfQuantum() ---- */
HIDDEN void endOfQuantum(){
	
	if(g_currentProc != NULL) // if were weren't finished,
	{
		insertProcQ(&(g_readyQueue), g_currentProc); // go back on the readyQueue
		// you're still ready, but go to the back of the line
		g_currentProc = NULL;
	}
	//setTIMER(QUANTUM); // refill the quantum
	scheduler(); // scheduler will reset timer, so no need to worry here
}

/* ---- getDeviceNumber() ---------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		Device Number
* Description:
*	Calculates the device number based on the Pending Interrupts
*	Bitmap.
*		
*	Same structure as getLineNumber, except we iterate over a
*	different bitmap.
*
*	1. Iteratively bit-wise & Pending Interrupts Bitmap with the
*	possible device number values in order.
*	2a.	If the device number matches, return it.
*	2b. If none do, PANIC!
*
*	The device number can be calculated from the Pending Interrupts Bitmap.
*	We determined this bitmap's address from the line number earlier.
*	We can bit-wise & this bitmap with possible device numbers.
* --------------------------------- end getDeviceNumber() ---- */

HIDDEN int getDeviceNumber(unsigned int *pendingIntMap){
	// 1. Traverse an array of potential device numbers and perform a bitwise & with that.
	// 		These are just hexadecimal equivalents of every possible
	// 		bit position being enabled independently.
	int bitArray[] = {DEVICEZERO, DEVICEONE, DEVICETWO, DEVICETHREE, DEVICEFOUR, DEVICEFIVE, DEVICESIX, DEVICESEVEN};
	
	// Check if device number corresponds, starting with highest priority, etc.
	for(int i = 0; i < TOTALDEVICES; i++)
	{
		if((*pendingIntMap & (bitArray[i])) != 0) // & with each one, return the one that matches!
		{
			return i; 		// 2a. Found it!
		}
	}
	PANIC(); // 2b. no device matches...
	// or should we handle this differently? maybe ignore? or return?
}

/* ---- getSemaphoreIndex() ---------------------------------------
* Parameters: 	None
* Type: 		Private
* Return:		Device Number
* Description:
*	Calculates the index at which a device's semaphore address is located
*	in the lotOfSemaphores.
*	(via the simplified line number and device number)
*		
*	Equation adapted from Principles of Operation page 36.
* --------------------------------- end getSemaphoreIndex() ---- */
int getSemaphoreIndex(int trueLineNumber, int deviceNumber){
	// We need to offset the line number since there are no external
	//	devices associated with lines 0-2.
	trueLineNumber = trueLineNumber - DEVICEOFFSET;
	
	// There are 8 devices per line (usually), and the index is
	//	offset by the device number
	int semaphoreIndex = (TOTALDEVICES * trueLineNumber) + deviceNumber;
	return semaphoreIndex;
}

/* ---- externalDeviceHandler() ---------------------------------------
* Parameters: 	Semaphore Index, and Line Number (simplified)
* Type: 		Private
* Return:		Interrupting Device's Status (in the signal proc's A1)
* Description:
*	Generically handle external device interrupts with
*	a V operation and turn off the alarm.
*	
*	The semaphore index is used to find the device's semaphore as well as helping
*	calculate the address of the interrupting device.
*
*	Line seven interrupts are handled based on their subdevice.
*	Transmission interrupts have higher priority and are handled first.
* --------------------------------- end externalDeviceHandler() ---- */

HIDDEN void externalDeviceHandler(int semaphoreIndex, int trueLineNumber){
	BOOL terminalMode = RECEIVING; // Default to receiving (if terminal). We'll change if neccessary.
	
	// We can get the device that was annoying us. Finally!
		// The calculation of a device's address has been adapted from page 36 of the Principles of Operation book.
	devreg_t* interruptingDevice = (devreg_t *) (DEVBASEADDRESS + (semaphoreIndex * DEVWORDLENGTH));

	// Check: Is it a terminal device?
	if (trueLineNumber == LINENUMSEVEN){ 

		// Case 1: We are transmitting (because we are ready to receive - pg 44, 45)
			// Isolate the ready status by comparing to binary sequence 11111111
		if((interruptingDevice->term.recv_status & ISOLATEREADY) == DEVICEREADY){
			semaphoreIndex = semaphoreIndex + TOTALDEVICES; // Increment the semaphore by 8
			// to accomodate for the fact we are looking at second subdevice
			terminalMode = TRANSMITTING;
		}

		// Case 2: We are transmitting (the default value)
	}
	

	// Now for the easy part - a V operation! Note that the semaphoreIndex points us to the semaphore address
	g_lotOfSemaphores[semaphoreIndex] = g_lotOfSemaphores[semaphoreIndex] + 1; // increment semAdd, as always
	
	if(g_lotOfSemaphores[semaphoreIndex] <= 0){ // // Someone was waiting on it
		
		// Signal the next guy
		pcb_PTR signaledProc = removeBlocked(&(g_lotOfSemaphores[semaphoreIndex]));
		
		// Case 1: Was a line 3-6 interrupt
		if (trueLineNumber != LINENUMSEVEN){

			if (signaledProc == NULL) { // If we couldn't wake someone...

				g_deviceStatus[semaphoreIndex] = interruptingDevice->dtp.status; // Let device know
			}

			signaledProc->p_semAdd = NULL;
			g_softBlockCount--;

			interruptingDevice->dtp.command = ACK; // Shut off the alarm
			signaledProc->p_s.a1 = interruptingDevice->dtp.status; // Return the status!
			
			insertProcQ(&(g_readyQueue), signaledProc); // Okay, on to the readyQueue
		}

		// Case 2: Was a line 7 interrupt
		else{
			if(signaledProc != NULL){ // Need to set some stuff up if we woke someone
				signaledProc->p_semAdd = NULL;
				g_softBlockCount--;
				
				// Need to ACK the right slot depending on whether we're sending
				//	or receiving. Also return appropriate status in A1.

				// Case 1: Was recieving
				if(terminalMode == RECEIVING){
					signaledProc->p_s.a1 = interruptingDevice->term.recv_status;
					interruptingDevice->term.recv_command = ACK;
				}
				
				// Case 2: Was transmitting
				else{
					signaledProc->p_s.a1 = interruptingDevice->term.transm_status;
					interruptingDevice->term.transm_command = ACK;
				}
				
				insertProcQ(&(g_readyQueue), signaledProc); // Okay, on to the readyQueue
			}
		}
	}

	// Finally, go back to where we left off!
	// Case 1: Someone was running when the interrupt was called
	if(g_currentProc != NULL){
		g_startTOD = getTODLO(); // If so, start the clock
		loadState(); // And load its state
	}
	
	// Case 2: No one running.
	scheduler(); // Need to get a new job.
}