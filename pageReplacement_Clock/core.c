/* Implementation of core functionality of the OS					*/
/* this includes the main scheduling loop							*/
/* for comments on the global functions see the associated .h-file	*/

/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bs_types.h"
#include "global.h"
#include "core.h"
#include "simruntime.h"

/* ----------------------------------------------------------------	*/
/* Declarations of global variables visible only in this file 		*/

PCB_t process;		// the only user process used for batch and FCFS
unsigned emptyFrameCounter;		// number of empty Frames 

/* ---------------------------------------------------------------- */
/*                Externally available functions                    */
/* ---------------------------------------------------------------- */

void initOS(void)
{
	systemTime = 0;						// reset the system time to zero
	initProcessTable();					// create the process table with empty PCBs
	srand( (unsigned)time( NULL ) );	// init the random number generator
	/* init the status of the OS */
	initMemoryManager();		// initialise the memory management system emptyFrameCounter = MEMSIZE;		// mark all memory as available
}

Boolean coreLoop(void)
{
	Boolean batchCompleted = FALSE;		// The batch has been completed 
	Boolean simError = FALSE;			// A severe, unrecoverable error occured in simulation
	PCB_t* nextProcess=NULL;			// pointer to process to schedule	
	PCB_t* pBlockedProcess = NULL;		// pointer to blocked process
	memoryEvent_t memoryEvent;			// action relevant to memory management
	memoryEvent_t *pMemoryEvent = NULL;	// pointer to that memory management event
	int frame = INT_MAX;				// physical address, neg. value indicate unrecoverable error

	do {	// loop until batch is complete
		pMemoryEvent = sim_ReadNextEvent(&memoryEvent);
		if (pMemoryEvent == NULL) break;			// on error exit the simulation loop 
		// advance time and run timer event handler if needed
		for (unsigned i = (systemTime / TIMER_INTERVAL); i < (pMemoryEvent->time / TIMER_INTERVAL); i++)
		{
			systemTime = (i+1) * TIMER_INTERVAL;  
			timerEventHandler();
		}
		systemTime = pMemoryEvent->time;	// set new system time according to next event
		
		// process the event that is due now
		switch (pMemoryEvent->action.op)
		{
		case start: 
			printf("%6u : PID %3u : Started\n", systemTime, pMemoryEvent->pid);
			// allocate the initial number of frames for the process
			createPageTable(pMemoryEvent->pid);
			break;
		case end:
			printf("%6u : PID %3u : Terminated\n", systemTime, pMemoryEvent->pid);
			// free all frames used by the process
			deAllocateProcess(pMemoryEvent->pid);
			break;
		case read: 
		case write:
			// event contains the page in use
			logPidMemAccess(pMemoryEvent->pid, pMemoryEvent->action);
			// resolve the location of the page in physical memory, this is the key function for memory management
			frame = accessPage(pMemoryEvent->pid, pMemoryEvent->action);
			// update memory mapping for simulation
			sim_UpdateMemoryMapping(pMemoryEvent->pid, pMemoryEvent->action, frame);
			logPidMemPhysical(pMemoryEvent->pid, pMemoryEvent->action.page, frame);
			break;
		default:
		case error:
			printf("%6u : PID %3u : ERROR in action coding\n", systemTime, pMemoryEvent->pid);
			break;
		}
		if (frame <0)	break;				// on error exit the simulation loop 
		logMemoryMapping();			
	} while (!batchCompleted && !simError);
	return batchCompleted; 
}
