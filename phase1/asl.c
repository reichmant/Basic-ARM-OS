/**************************************************************
* FILENAME:		asl.c
* 
* DESCRIPTION:	Active Semaphore List Module for JaeOS
* 
* NOTES:		This module contains two NULL-terminated single linearly linked lists
*				of semaphore descriptors. These lists keep track of Active Semaphores
*				and free semaphores. The ASL is sorted in ascending order
*				using the s_semAdd field as the sort key.
*
*				This module contains the functions neccessary to move the semaphores
*				between lists when they become (in)active.
*				A semaphore is defined as "active" if there is at least one ProcBlk
*				on the process queue associated it.
* 
* AUTHORS:		Thomas Reichman; Ajiri Obaebor
*				Some descriptions adapted from Michael Goldweber
*				Additional help from Peter Rozzi, Patrick Gemperline, and Neal Troscinski
*				C commenting conventions adapted from http://syque.com/cstyle/ch4.htm
**************************************************************/

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"

///////////////////////// DEFINITONS //////////////////////////

// Semaphore Descriptor
typedef struct semd_t {
	struct semd_t 	*s_next;		// next element on the ASL
	int 			*s_semAdd;		// pointer to the semaphore
	pcb_t 			*s_procQ;		// tail pointer to a process queue
} semd_t;

// Semaphore Lists
HIDDEN semd_t *semd_h, *semdFree_h;
	// semd_h: Active Semaphore list
	// semdFree_h: Semaphore Free List

//////////////////// FUNCTION DECLARATIONS ////////////////////
/********************* Public Functions **********************/
void initASL();
int insertBlocked(int *semAdd, pcb_PTR p);
pcb_PTR removeBlocked(int *semAdd);
pcb_PTR outBlocked(pcb_PTR p);
pcb_PTR headBlocked(int *semAdd);
/********************* Private Functions *********************/
HIDDEN semd_t *findPrevSemd(int *semAdd);
HIDDEN void freeSemd(semd_t *semd);
HIDDEN semd_t *allocateSemd();
////////////////////// End Declarations ///////////////////////


////////////////////// Public Functions ///////////////////////

/* ---- initASL() ---------------------------------------------
* Parameters: 	int *semAdd
* Type: 		Public
* Return:		None
* Description:
*	Initialize the semdFree list to contain all the elements of
*	the array static semd_t semdTable[MAXPROC].
*	This method will be only called once during
*	data structure initialization.
* --------------------------------------- end initASL() ---- */
void initASL(){
	semd_t *topDummyNode;
	semd_t *bottomDummyNode;
	// Initialize the static array of semaphores, including both dummy nodes
	static semd_t semdTable[(MAXPROC + 2)];
	semdFree_h = NULL;

	// Iteratively put the on the Semaphore Free List
	for (int i = 0; i < (MAXPROC + 2); i++) {
		freeSemd(&(semdTable[i]));
	}
	
	// Manually allocate the dummy nodes
	topDummyNode = allocateSemd();
	topDummyNode->s_next = NULL;
	topDummyNode->s_semAdd = 0;

	bottomDummyNode = allocateSemd();
	bottomDummyNode->s_next = NULL;
	bottomDummyNode->s_semAdd = (int *) 0xFFFFFF;

	semd_h = topDummyNode;	// Set the head to the top dummy node
							// Set the tail to the bottom dummy node?
}

/* ---- insertBlocked() ---------------------------------------
* Parameters: 	int *semAdd, pcb_PTR p
* Type: 		Public
* Return:		Boolean
* Description:
*	Insert the ProcBlk pointed to by p at the tail of the process
*	queue associated with the semaphore whose physical address is
*	semAdd and set the semaphore address of p to semAdd.
*
*	If the semaphore is currently not active
*	(i.e. there is no descriptor for it in the ASL),
*	allocate a new descriptor from the semdFree list,
*	insert it in the ASL (at the appropriate position),
*	initialize all of the fields
*	(i.e. set s_semAdd to semAdd, and s_procQ to mkEmptyProcQ()),
*	and proceed as above.
*	If a new semaphore descriptor needs to be allocated and
*	the semdFree list is empty, return TRUE.
*	In all other cases return FALSE.
* --------------------------------- end insertBlocked() ---- */
int insertBlocked(int *semAdd, pcb_PTR p) {

	// Initiailze semaphore pointer to be added
	semd_t *newSemd = NULL;

	// Get ahold of the previous semaphore
	semd_t *prevSemd = findPrevSemd(semAdd);
	
	// Is the previous semaphore active?
	if ((prevSemd->s_next == NULL) || (prevSemd->s_next->s_semAdd != semAdd)) {
	// Case 1: It isn't active!
		// Get a semaphore from the free list so we can allocate it
		newSemd = allocateSemd();

		// Make sure it isn't NULL
		if (newSemd == NULL) {
			return TRUE;
		}
		// Populate the attributes
		newSemd->s_semAdd = semAdd;
		p->p_semAdd = newSemd->s_semAdd;
		newSemd->s_procQ = mkEmptyProcQ();
		// Ready to put in the ProcQ
		insertProcQ(&(newSemd->s_procQ), p);

		// typical weaving
		newSemd->s_next = prevSemd->s_next;
		prevSemd->s_next = newSemd;

		return FALSE;
	}
	// Case 2: It's active!
	// Ready to put in the ProcQ
	insertProcQ(&(prevSemd->s_next->s_procQ), p);

	// Update the semaphore address
	p->p_semAdd = semAdd;
	return FALSE;
}

/* ---- removeBlocked() ---------------------------------------
* Parameters: 	int *semAdd
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Search the ASL for a descriptor of this semaphore.
*	If none is found, return NULL; otherwise,
*	remove the first (i.e. head) ProcBlkfrom the process queue
*	of the found semaphore descriptor and return a pointer to it.
*	
*	If the process queue for this semaphore becomes empty
*	(emptyProcQ(sprocq)is TRUE), remove the semaphore descriptor
*	from the ASL and return it to the semdFree list.
* --------------------------------- end removeBlocked() ---- */
pcb_PTR removeBlocked(int *semAdd) {
	// Get the previous Semd
	semd_t *prevSemd = findPrevSemd(semAdd);

	// Error Case: Assert that it actually exists and that we have the right one
	if ( (prevSemd->s_next->s_semAdd != semAdd) || (prevSemd->s_next == NULL)) {
		return (NULL);
	}

	// Since we found it, we can remove it.
	// This will be returned, but first we may have to do some cleanup
	pcb_PTR retPcb = removeProcQ(&(prevSemd->s_next->s_procQ));	
	
	// Case 1: ProcessQueue is empty - time for deallocation!
	if (emptyProcQ(prevSemd->s_next->s_procQ)) {
		// Get ahold of semaphore to be removed and unweave
		semd_t *retSemd = prevSemd->s_next; 
		prevSemd->s_next = retSemd->s_next;
		retSemd->s_next = NULL;
		// Take node out of active list and put back on freeList
		freeSemd(retSemd);
	}
	// Case 2: ProcessQueue is not empty: you're done
	return retPcb;	// return regardless of above cases
}

/* ---- outBlocked() ------------------------------------------
* Parameters: 	pcb_PTR p
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Remove the ProcBlk pointed to by p from the process
*	queue associated with p’s semaphore (p→psemAdd) on the ASL.
*	If ProcBlk pointed to by p does not appear in the process queue
*	associated with p’s semaphore, which is an error condition,
*	return NULL; otherwise, return p.
* ------------------------------------ end outBlocked() ---- */
pcb_PTR outBlocked(pcb_PTR p) {
	// Get the previous Semd
	semd_t *prevSemd = findPrevSemd(p->p_semAdd);

	// Error Case: Assert that it actually exists and that we have the right one
	if ( (prevSemd->s_next->s_semAdd != p->p_semAdd) || (prevSemd->s_next == NULL)) {
		return (NULL);
	}

	// Since we found it, we can remove it from the semaphore's queue
	// This will be returned, but first we may have to do some cleanup
	pcb_PTR retPcb = outProcQ(&(prevSemd->s_next->s_procQ), p);
	// Assert that we actually got something in return
	if (retPcb == NULL) {
		return (NULL);
	}
	// Case 1: ProcessQueue is empty - time for deallocation!
	if (emptyProcQ(prevSemd->s_next->s_procQ)) {
		// Get ahold of semaphore to be removed and unweave
		semd_t *retSemd = prevSemd->s_next;
		prevSemd->s_next = retSemd->s_next;
		retSemd->s_next = NULL;
		// Take node out of active list and put back on freeList
		freeSemd(retSemd);
	}
	// Case 2: ProcessQueue is not empty: you're done
	return retPcb;	// return regardless of above cases
}

/* ---- headBlocked() -----------------------------------------
* Parameters: 	int *semAdd
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Return a pointer to the ProcBlk that is at the head of the
*	process queue associated with the semaphore semAdd.
*	Return NULL if semAdd is not found on the ASL or if the process
*	queue associated with semAdd is empty.
* ----------------------------------- end headBlocked() ---- */
pcb_PTR headBlocked(int *semAdd) {
	
	// Get the previous Semd
	semd_t *prevSemd = findPrevSemd(semAdd);

	// Error Case: Assert that it actually exists and that we have the right one
	if ( (prevSemd->s_next->s_semAdd != semAdd) || (prevSemd->s_next == NULL)) {
		return (NULL);
	}
	
	// Get the head PCB and return it
	return headProcQ(prevSemd->s_next->s_procQ);
}

///////////////////// Private and Helper Functions /////////////////////

/* ---- findPrevSemd() ----------------------------------------
* Parameters: 	int *semAdd
* Type: 		Private
* Return:		pcb_PTR or NULL
* Description:
*	Search method - given a semd, find a pointer to semd preceding
*	node we're looking for or node preceding where it would be.
* ---------------------------------- end findPrevSemd() ---- */
HIDDEN semd_t *findPrevSemd(int *semAdd) {
	semd_t *currentSemd = semd_h; // Start off at the head of the list (top dummy node)
		
	// Traverse down. We can stop looking once either of these conditions are true:
	// 	The next semaphore address is NULL (since that means we're at the end)
	//	The next semaphore address is greater than the one we're looking for (since it's sorted)
	while ( (currentSemd->s_next->s_semAdd < semAdd) && (currentSemd->s_next != NULL) ) {
			currentSemd = currentSemd->s_next;
	}
	return currentSemd;
}

/* ---- freeSemd() --------------------------------------------
* Parameters:	semd_t *semd
* Type:			Private
* Return:		None
* Description:
*	Move a semd into the free list from the active list
* -------------------------------------- end freeSemd() ---- */
HIDDEN void freeSemd(semd_t *semd) {
	// Case 1: Nothing on the stack yet
	if(semdFree_h == NULL) {
		semdFree_h = semd;
		semdFree_h->s_next = NULL;
	}
	// Case 2: Something already on the stack
	else {
		semd->s_next = semdFree_h;
		semdFree_h = semd;
	}
}

/* ---- allocateSemd() --------------------------------------------
* Parameters:	None
* Type:			Private
* Return:		freeSemd
* Descripton:
* 	Move a semaphore from the free list to the active semaphore list
* -------------------------------------- end allocateSemd() ---- */
HIDDEN semd_t *allocateSemd() {
	// Case 1: No semaphores are on the free list
	if (semdFree_h == NULL) {
		return (NULL);
	}
	// Case 2: There are semaphores on the free list
	semd_t *freeSemd = semdFree_h;
	if (semdFree_h->s_next == NULL) {
		// Case 2a: There's only one semaphore on the free list
		semdFree_h = NULL; // Nullify the list since it's empty now.
	}
	else {
		// Case 2b: There are other semaphores on the free list - adjust the head pointer
		semdFree_h = semdFree_h->s_next;
		freeSemd->s_next = NULL;
	}
	// Clear the structure's attributes and return it
	freeSemd->s_next = NULL;
	freeSemd->s_semAdd = NULL;
	freeSemd->s_procQ = NULL;
	return freeSemd;	
}