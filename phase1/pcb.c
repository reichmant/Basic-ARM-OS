/**************************************************************
* FILENAME:		pcb.c
* 
* DESCRIPTION:	Process Control Block Module for JaeOS
* 
* NOTES:		This module contains functions to help manage the queues
*				of containing running processes. This includes initialization
*				removal, and other management functionality.
*
* AUTHORS:		Thomas Reichman; Ajiri Obaebor
*				Some descriptions adapted from Michael Goldweber
*				Additional help from Peter Rozzi, Neal Troscinski
*				C commenting conventions adapted from http://syque.com/cstyle/ch4.htm
**************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"

///////////////////////// DEFINITONS //////////////////////////
pcb_PTR pcbList_h;			// Initialize PCB free list with a pointer to its head
//////////////////// FUNCTION DECLARATIONS ////////////////////
/********************* Public Functions **********************/
pcb_PTR allocPcb();
void freePcb (pcb_PTR p);
void initPcbs();
int emptyProcQ(pcb_PTR tp);
pcb_PTR mkEmptyProcQ();
pcb_PTR headProcQ(pcb_PTR tp);
void insertProcQ(pcb_PTR *tp, pcb_PTR p);
pcb_PTR removeProcQ(pcb_PTR *tp);
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p);
int emptyChild(pcb_PTR p);
void insertChild(pcb_PTR prnt, pcb_PTR p);
pcb_PTR removeChild(pcb_PTR prnt);
pcb_PTR outChild(pcb_PTR p);
////////////////////// End Declarations ///////////////////////


////////////////////// Public Functions ///////////////////////

/* ---- allocPcb() --------------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Return NULL if the pcbFree list is empty. 	
*	Otherwise, removean element from the 		
*	pcbFree list, provide initial values for ALL 
*	of the ProcBlk’s fields (i.e. NULL and/or 0) 
*	and then return a pointer to the removed 		
*	element. ProcBlk’s get reused, so it is 		
*	important that no previous values persist in 
*	a ProcBlk when itgets reallocated.
* -------------------------------------- end allocPcb() ---- */
pcb_PTR allocPcb(){
	if (pcbList_h == NULL){
		return (NULL);
	}
	pcb_PTR unusedPCB = removeProcQ(&(pcbList_h));

	//Reset ALL of p's pointers to NULL
	unusedPCB->p_next = NULL;
	unusedPCB->p_prev = NULL;
	unusedPCB->p_prnt = NULL;
	unusedPCB->p_child = NULL;
	unusedPCB->p_nextSib = NULL;
	unusedPCB->p_prevSib = NULL;	

	//PHASE 2 STUFF
	unusedPCB->p_time = 0; // microseconds

	return unusedPCB;
}

/* ---- freePcb() ---------------------------------------------
* Parameters: 	pcb_PTR p
* Type: 		Public
* Return:		None
* Description:
*	Insert the element pointed to by 
*	p onto the pcbFree list.  
* --------------------------------------- end freePcb() ---- */
void freePcb (pcb_PTR p) {
	// More effecient to procrasinate the dishes!
	insertProcQ(&(pcbList_h), p);
}

/* ---- initPcbs() --------------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		None
* Description:
*	Initialize the pcbFree list to contain
*	all the elements ofthestatic array of 
*	MAXPROC ProcBlk’s. This method will be
*	called only once during data structure 
*	initialization. 						   
* -------------------------------------- end initPcbs() ---- */
void initPcbs() {
	static pcb_t procTable[MAXPROC];

	pcbList_h = mkEmptyProcQ(); // Create the list

	for (int i=0; i<MAXPROC; i++) { // Iteratively populate it
		freePcb (&(procTable[i]));
	}	

}

/* ---- emptyProcQ() ------------------------------------------
* Parameters: 	pcb_PTR tp
* Type: 		Public
* Return:		Boolean
* Description:
*	Check if Queue (whose tail is pointed to by tp) is empty.						   
* ------------------------------------ end emptyProcQ() ---- */
int emptyProcQ(pcb_PTR tp) {
	return (tp == NULL);
}

/* ---- mkemptyProcQ() ----------------------------------------
* Parameters: 	None
* Type: 		Public
* Return:		NULL (see description)
* Description:
*	Initialize a variable to be tail pointer
*	to a process queue. Return a pointer to the tail of an empty
*	process queue; i.e. NULL. 
* ---------------------------------- end mkemptyProcQ() ---- */
pcb_PTR mkEmptyProcQ(){
	return (NULL);
}

/* ---- headProcQ() -------------------------------------------
* Parameters: 	pcb_PTR tp
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Return a pointer to the first ProcBlk  
*	from the process queue whose tail is   
*	pointed to by tp. Do not remove this   
*	ProcBlk from the process queue. Return 
*	NULL if the process queue is empty.
* ------------------------------------- end headProcQ() ---- */
pcb_PTR headProcQ(pcb_PTR tp) {
	if (emptyProcQ(tp)){
	 return (NULL);
	}
	return (tp->p_next);
}

/* ---- insertProcQ() -----------------------------------------
* Parameters: 	pcb_PTR *tp, pcb_PTR p
* Type: 		Public
* Return:		None
* Description:
*	Insert the ProcBlk pointed to by p into the  
*	process queue whose tail-pointer is pointed 
*	to by tp. Note the double indirection      
*	through tp to allow for the possible       
*	updating of the tail pointer as well.
* ----------------------------------- end insertProcQ() ---- */
void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	
	
	// Case 1: Queue is empty
	if (emptyProcQ(*tp)){			// If it IS empty, deal with that by making the next/prev itself
		p->p_next = p;
		p->p_prev = p;
	}

	// Case 2: Queue is NOT empty
	else{
	// Carefully insert the new pcb by adjusting:
		p->p_next = (*tp)->p_next;	// the next value of the new element
		(*tp)->p_next->p_prev = p;	// the prev value of next value of the tail pointer
		(*tp)->p_next = p;			// the next value of the tail pointer
		p->p_prev = *tp;			// the prev value of the new element
	}

	*tp = p;						// Always redirect tail pointer to newly inserted element because we insert to end of a queue
}

/* ---- removeProcQ() -----------------------------------------
* Parameters: 	pcb_PTR *tp
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Remove the first (i.e. head) element from the  
*	process queue whose tail-pointer is pointed to 
*	by tp. Return NULL if the process queue was    
*	initially empty; otherwise return the pointer  
*	to the removed element. Update the process     
*	queue’s tail pointer if necessary.             
* ----------------------------------- end removeProcQ() ---- */
pcb_PTR removeProcQ(pcb_PTR *tp){
	// Case 1: 0 Elements in the queue
	if (emptyProcQ(*tp)){
		return (NULL);
	}
	// Case 2: 1 element in tp
	if ((*tp)->p_next == (*tp)){
		pcb_t* ret = *tp;
		*tp = NULL;
		return ret;
	}
	// Case 3: 2+ elements in tp
	pcb_t* ret = (*tp)->p_next;
	(*tp)->p_next = (*tp)->p_next->p_next;
	(*tp)->p_next->p_prev = (*tp);
	ret -> p_next = NULL;
	ret -> p_prev = NULL;
	return ret;
}

/* ---- outProcQ() --------------------------------------------
* Parameters: 	pcb_PTR *tp, pcb_PTR p
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Remove the ProcBlk pointed to by p from the process   
*	queue whose tail-pointer is pointed to by tp. Update  
*	the process queue’s tail pointer if necessary. If the 
*	desired entry is not in the indicated queue (an       
*	error condition), return NULL; otherwise, return p.   
*	Note that p can point to any element of the process   
*	queue.												           
* -------------------------------------- end outProcQ() ---- */
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p){
	// Case 1: The queue is empty. Assert that we can't remove from an empty queue
	if (emptyProcQ(*tp)){
		return (NULL);
	}
	// Case 2: The node to remove IS the tail pointer
	if (p == *tp){
		// If tail pointer's next ISN't itself, bridge gap between its prev and next
		// Adjust the tail pointer to the previous ProcBlk since we're about to remove the current one
		if((*tp)->p_next != *tp){					
			(*tp)->p_next->p_prev = (*tp)->p_prev;
			(*tp)->p_prev->p_next = (*tp)->p_next;
			*tp = (*tp)->p_prev;						
		}
		else{
			*tp = NULL;
		}
		return p;
	}
	// Case 3: 	The node to remove ISN'T the tail pointer
	else{
			pcb_PTR pcbAfterTP = (*tp)->p_next;
			while (pcbAfterTP != *tp){	
				// If the pcb after the tail pointer points at the node we want to remove
				if (pcbAfterTP == p){
					// Bridge the gap, since we're about to remove it
					pcbAfterTP->p_next->p_prev = pcbAfterTP->p_prev;
					pcbAfterTP->p_prev->p_next = pcbAfterTP->p_next;
					// Don't need these anymore since it's gone now
					pcbAfterTP->p_next = NULL;
					pcbAfterTP->p_prev = NULL;
	
					return pcbAfterTP;
				}
				pcbAfterTP = pcbAfterTP->p_next; // Traverse to next node and try again until we find it!
			}
			// Error Condition: The node to be removed was not found, return NULL
			return (NULL);
	}

}

/* ---- emptyChild() ------------------------------------------
* Parameters: 	pcb_PTR p
* Type: 		Public
* Return:		Boolean
* Description:
*	Return TRUE if the ProcBlk pointed to by p has no children.
*	Return FALSE otherwise.  											           
* ------------------------------------ end emptyChild() ---- */
int emptyChild(pcb_PTR p){
	return (p->p_child == NULL);
}

/* ---- insertChild() -----------------------------------------
* Parameters: 	pcb_PTR prnt, pcb_PTR p
* Type: 		Public
* Return:		None
* Description:
*	Make the ProcBlk pointed to by p a child of the ProcBlk
*	pointedto by prnt.
* ----------------------------------- end insertChild() ---- */
void insertChild(pcb_PTR prnt, pcb_PTR p){
	// Case 1: p has no children
	if(emptyChild(prnt)){
		p->p_prevSib = NULL; // Therefore, it has no previous sibling
	}
	// Case 2: p already has children
	else{
		// p becomes a sibling to existing children
		prnt->p_child->p_nextSib = p;
		p->p_prevSib = prnt->p_child;
	}

	// p has no next sibling so it was most recently "birthed"
	p->p_nextSib = NULL;

	// The new child is the parent's "first" child now
	prnt->p_child = p;
	p->p_prnt = prnt;

}

/* ---- removeChild() -----------------------------------------
* Parameters: 	pcb_PTR prnt
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Make the first child of the ProcBlk pointed to        
*	by p no longer a child of p. Return NULL if           
*	initially there were no children of p. Otherwise,     
*	return a pointer to this removed first child ProcBlk. 
* ----------------------------------- end removeChild() ---- */
pcb_PTR removeChild(pcb_PTR prnt){
	// Case 1: The parent has no children - nothing to remove
	if (emptyChild(prnt)){
		return NULL;
	}

	// Case 2: The parent has at least one child that can be removed.
	pcb_PTR firstChild =  prnt->p_child;	// We are only interested in the first child
	
	// Make sure it's the first child by asserting it has no previous sibling
	if (firstChild->p_prevSib == NULL){
		
		// Unlink the child from its parent, you monster
		firstChild->p_prnt = NULL;
		prnt->p_child = NULL;

		return firstChild;
	}
	// In the event that firstChild has a previous sibling, correct the parent's p_child to point to it
	prnt->p_child = firstChild->p_prevSib;

	// Assuming that this is now the TRUE firstchild, we can remove it from the parent,
	// unlink it from siblings, etc.
	firstChild->p_prevSib->p_nextSib = NULL;
	firstChild->p_prevSib = NULL;
	firstChild->p_prnt = NULL;
	return firstChild;
}

/* ---- outChild() --------------------------------------------
* Parameters: 	pcb_PTR p
* Type: 		Public
* Return:		pcb_PTR or NULL
* Description:
*	Make the ProcBlk pointed to by p no longer 
*	the child of its parent. If the ProcBlk    
*	pointed to by p has no parent, return NULL;
*	otherwise, return p. Note that the element 
*	pointed to by p need not be the first      
*	child of its parent.						  
* -------------------------------------- end outChild() ---- */
pcb_PTR outChild(pcb_PTR p){
// Case 1: p has no parent
	if(p->p_prnt == NULL){
		return (NULL);
	}
// Case 2: p has a parent
	// Case 2a: p is the child of the parent
	if(p == p->p_prnt->p_child){
		// unweave from sibling case, pay attention to boundry cases
		// Return p as no longer the child of its parent
		return removeChild(p->p_prnt);
	}
	// Case 2b: p is NOT the child of the parent
		// Case 2b I: p is the first child (boundary case: no previous sibling(s))
	if (p->p_prevSib == NULL){
		p->p_nextSib->p_prevSib = NULL;
	}
		// Case 2b II: p is not the first child (previous sibling(s))
	else{
		p->p_nextSib->p_prevSib = p->p_prevSib;
		p->p_prevSib->p_nextSib = p->p_nextSib;
		p->p_prevSib = NULL;
	}
	// Either way, we still have to reset these attributes
	p->p_nextSib = NULL;
	p->p_prnt = NULL;
	return p;
}