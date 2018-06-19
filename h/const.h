#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * All constants are defined here
 * 
 ****************************************************************************/
#include "/usr/include/uarm/arch.h"
#include "/usr/include/uarm/uARMconst.h"

// Boolean Aliases
#define BOOL				int
#define	TRUE				1
#define	FALSE				0

// Helper functions
#define	HIDDEN				static

// Hardcoded max number of processes
#define MAXPROC  			20

// Cause Register Aliases
// REMEMBER, 0 IS ENABLED, 1 IS DISABLED!!!
#define ALLOFF				0x00000000
#define INTSDISABLED		0x000000C0		// last 12 bits(?)
#define INTSENABLED			0xFFFFFF3F



// CPSR MODES - page 10 PrinciplesOfOperation
#define USERMODE			0x00000010
#define FASTINTMODE			0x00000011
#define INTMODE				0x00000012
#define SUPERVISORMODE		0x00000013
#define ABORTMODE			0x00000017
#define UNDEFMODE			0x0000001B
#define SYSMODE 			0x0000001F 		// last 5 bits

// Stored Processor States (page 24 PrinciplesOfOperation)
#define INTOLDADD			0x00007000
#define INTNEWADD	 		0x00007058
#define TLBOLDADD			0x000070B0
#define TLBNEWADD			0x00007108
#define PGMTOLDADD			0x00007160
#define PGMTNEWADD			0x000071B8
#define SYSOLDADD			0x00007210
#define SYSNEWADD			0x00007268

// Miscellaneous
#define MAXSEMA4			49
#define MAXNUMDEV			49 			// one device per sema4, but different name so context is understood
#define PCPREFETCH			4
#define RI					20 			// Reserved Instruction - page 9

// Time Related
#define QUANTUM				5000 		// full CPU burst in microseconds
#define INTERVAL			100000		// full interval timer in microseconds
#define CLOCKINDEX			48 			// the last device

// SYS call numbers
#define CREATEPROCESS		1
#define TERMINATEPROCESS	2
#define VERHOGEN			3
#define PASSEREN			4
#define SPECTRAPVEC			5
#define GETCPUTIME			6
#define WAITCLOCK			7
#define	WAITIO				8

// Trap Types
#define TLBTRAP				0
#define PGMTRAP				1
#define SYSTRAP				2

// Task Completion States
// (mostly for CREATEPROCESS)
#define SUCCESS				0
#define FAILURE				-1

// Simplified Line Numbers: 0-7 as integerss
#define LINENUMZERO			0
#define LINENUMONE			1
#define LINENUMTWO			2
#define LINENUMTHREE		3
#define LINENUMFOUR			4
#define LINENUMFIVE			5
#define LINENUMSIX			6
#define LINENUMSEVEN		7

// Actual Line Numbers
// NOTE: These are just hexadecimal equivalents of every possible
// 	bit position being enabled independently.
										// BINARY EQUIVALENT
#define LINEZERO			0x00000001 	// 00000001
#define LINEONE				0x00000002	// 00000010
#define LINETWO				0x00000004	// 00000100
#define LINETHREE			0x00000008	// 00001000
#define LINEFOUR			0x00000010	// 00010000
#define LINEFIVE			0x00000020	// 00100000
#define LINESIX				0x00000040	// 01000000
#define LINESEVEN			0x00000080	// 10000000

// Device Numbers
// NOTE: These are just hexadecimal equivalents of every possible
// 	bit position being enabled independently.
// It's the same as above with a context-specific name.
										// BINARY EQUIVALENT
#define DEVICEZERO			0x00000001 	// 00000001
#define DEVICEONE			0x00000002	// 00000010
#define DEVICETWO			0x00000004	// 00000100
#define DEVICETHREE			0x00000008	// 00001000
#define DEVICEFOUR			0x00000010	// 00010000
#define DEVICEFIVE			0x00000020	// 00100000
#define DEVICESIX			0x00000040	// 01000000
#define DEVICESEVEN			0x00000080	// 10000000

// Terminal Stuff
#define DEVICEREADY			1
#define ACK					1
#define RECEIVING			TRUE
#define TRANSMITTING		FALSE
#define ISOLATEREADY		0x0000000F	// 11111111 in binary

// Device Related
#define DEVICEOFFSET		3
#define TOTALDEVICES		8
#define TOTALLINENUMS		8
#define DEVBASEADDRESS		0x00000040
#define DEVWORDLENGTH		0x00000010
#define LASTSEMINDEX		48


// Pending Iterrupt Bitmap (p. 21 Principles)
#define TERMINALINTMAP		0x00006FF0
#define PRINTERINTMAP		0x00006FEC
#define NETWORKINTMAP		0x00006FE8
#define TAPEINTMAP			0x00006FE4
#define DISKINTMAP			0x00006FE0

#define LINENUMOFFSET		24

#endif
