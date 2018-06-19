#ifndef TYPES
#define TYPES

/**************************************************************************** 
 *
 * All type definitions go here
 * 
 ****************************************************************************/
#include "./const.h"


//  #include "/usr/include/uarm/uARMtypes.h"
//  ^ copy whats needed - commented out to avoid redefine warnings


/************************** Device register types ***************************/
// Copied from uARMtypes.h
// DEVICE REGISTER FIELDS - pg. 35, 36
// More about how these are used for terminal (sub)devices
 // can be found on page 43 of the Principles of Operation text.

// Calculation of the address where these are located is on
 // page 36 of the Principles of Operation text.

/* Device register type for disks, tapes and printers (dtp) */
typedef struct {
  unsigned int status;
    unsigned int command;
    unsigned int data0;
    unsigned int data1;
} dtpreg_t;

/* Device register type for terminals; fields have different meanings */
typedef struct {
  unsigned int recv_status;
    unsigned int recv_command;
    unsigned int transm_status;
    unsigned int transm_command;
} termreg_t;

/* With a pointer to devreg_t we can refer to a "generic" device register, and
   then use the appropriate union member to access the fields (not strictly
     necessary, but for convenience and clarity) */

// Basically, we change the names for context.
    // The union structure just means they are defined the same way.

typedef union {
  dtpreg_t dtp;
  termreg_t term;
} devreg_t;

typedef struct{
    unsigned int a1;    //r0
    unsigned int a2;    //r1
    unsigned int a3;    //r2
    unsigned int a4;    //r3
    unsigned int v1;    //r4
    unsigned int v2;    //r5
    unsigned int v3;    //r6
    unsigned int v4;    //r7
    unsigned int v5;    //r8
    unsigned int v6;    //r9
    unsigned int sl;    //r10
    unsigned int fp;    //r11
    unsigned int ip;    //r12
    unsigned int sp;    //r13
    unsigned int lr;    //r14
    unsigned int pc;    //r15
    unsigned int cpsr;
    unsigned int CP15_Control;
    unsigned int CP15_EntryHi;
    unsigned int CP15_Cause;
    unsigned int TOD_Hi;
    unsigned int TOD_Low;
}state_t;

// The two types of states for an process' area
typedef struct p_states {

    state_t *oldState;
    state_t *newState;

} p_states;

 typedef struct pcb_t {
     struct pcb_t   *p_next;    
     struct pcb_t   *p_prev;    
 
     struct pcb_t   *p_prnt,    
            *p_child,           
            *p_nextSib,         
            *p_prevSib;         
    
     state_t    p_s;              
     int        p_time;           
     int        *p_semAdd;        
     p_states   stateArray[3]; // Each of the three types of traps
                                // is associated with two areas
 }  pcb_t, *pcb_PTR;
 
#endif
