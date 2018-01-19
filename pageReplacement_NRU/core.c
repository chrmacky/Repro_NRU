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
memoryEventStack stack = NULL;
memoryEventStackEntry *firstStackElement = NULL;
memoryEventStackEntry *lastStackElement = NULL;

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
	PCB_t* nextProcess = NULL;			// pointer to process to schedule	
	PCB_t* pBlockedProcess = NULL;		// pointer to blocked process
	memoryEvent_t memoryEvent;			// action relevant to memory management
	memoryEvent_t *pMemoryEvent = NULL;	// pointer to that memory management event
	int frame = INT_MAX;
	do {	// loop until batch is complete
		pMemoryEvent = sim_ReadNextEvent(&memoryEvent);
		if (pMemoryEvent == NULL) break;			// on error exit the simulation loop 
		// advance time and run timer event handler if needed
		for (unsigned i = (systemTime / TIMER_INTERVAL); i < (pMemoryEvent->time / TIMER_INTERVAL); i++)
		{
			systemTime = (i + 1) * TIMER_INTERVAL;
			timerEventHandler();
		}
		systemTime = pMemoryEvent->time;	// set new system time according to next event

		// call that controls the execution of the process
		frame = executeMemoryEvent(pMemoryEvent);

		if (frame < 0)	break;				// on error exit the simulation loop 

		logMemoryMapping();
	} while (!batchCompleted && !simError);
	return batchCompleted;
}

int executeMemoryEvent(memoryEvent_t *pMemoryEvent) {
	int frame = INT_MAX;				// physical address, neg. value indicate unrecoverable error
	// process the event that is due now
	switch (pMemoryEvent->action.op)
	{
	case start:
		if (processTable[pMemoryEvent->pid].pageTable == NULL){
			printf("%6u : PID %3u : Started\n", systemTime, pMemoryEvent->pid);
			// allocate the initial number of frames for the process
			createPageTable(pMemoryEvent->pid);
		}
		else {
			//cannot start process right now -> process already started before and is running
			printf("%6u : PID %3u : <<< ...process is already running... >>>\n", systemTime, pMemoryEvent->pid);
		}
		break;
	case end:
		if (processTable[pMemoryEvent->pid].pageTable != NULL) 
		{
			printf("%6u : PID %3u : Terminated\n", systemTime, pMemoryEvent->pid);
			// free all frames used by the process
			deAllocateProcess(pMemoryEvent->pid);
		}
		else 
		{
			// process has already ended before and is not longer running
			printf("%6u : PID %3u : Process not namend\n", systemTime, pMemoryEvent->pid);
		}
		break;
	case read:
	case write:
		if (processTable[pMemoryEvent->pid].pageTable != NULL) {
			if (!isMemoryAvailable(pMemoryEvent->pid)) {
				//There is no space in memory for the requested action
				PushToStack(pMemoryEvent);
			}
			else {
				// event contains the page in use
				logPidMemAccess(pMemoryEvent->pid, pMemoryEvent->action);
				// resolve the location of the page in physical memory, this is the key function for memory management
				frame = accessPage(pMemoryEvent->pid, pMemoryEvent->action);
				// update memory mapping for simulation
				sim_UpdateMemoryMapping(pMemoryEvent->pid, pMemoryEvent->action, frame);
				logPidMemPhysical(pMemoryEvent->pid, pMemoryEvent->action.page, frame);
			}
		}
		else 
		{
			// process has already ended before and is not longer running
			printf("%6u : PID %3u : Process not namend\n", systemTime, pMemoryEvent->pid);
		}
		break;
	default:
	case error:
		printf("%6u : PID %3u : ERROR in action coding\n", systemTime, pMemoryEvent->pid);
		break;
	}
	if (isMemoryAvailable(INT_MAX)) {
		frame = executeFromStack(frame);
	}
	return frame;
}

int executeFromStack(int frame) {
	if (firstStackElement == NULL || stack == NULL)
		return frame;
	int resultFrame = frame;

	memoryEventStack *toBeDeleted = NULL;

	while (firstStackElement != NULL)
	{
		memoryEvent_t *memevent = NULL;
		memevent = malloc(sizeof(memoryEvent_t));
		memevent->pid = firstStackElement->pid;
		memevent->time = firstStackElement->time;
		/*action_t *act = NULL;
		act = malloc(sizeof(action_t));
		act->op = firstStackElement->action->op;
		act->page = firstStackElement->action->page;
		
		memcpy(&act, &firstStackElement->action, sizeof(action_t));
		memevent->action = *act;*/
		memevent->action = *firstStackElement->action;

		if (isMemoryAvailable(memevent->pid)){
			resultFrame = executeMemoryEvent(memevent, FALSE);
			toBeDeleted = firstStackElement;
			if (firstStackElement == lastStackElement || firstStackElement->next == NULL){
				stack = NULL;
				firstStackElement = NULL;
				lastStackElement = NULL;
				break;
			}
			firstStackElement = firstStackElement->next;
			//free(toBeDeleted);
		}
		else break;
	}
	return resultFrame;
}

Boolean PushToStack(memoryEvent_t *pmemoryEvent) {
	memoryEventStackEntry *item = NULL;
	item = malloc(sizeof(memoryEventStackEntry));
	action_t *act = NULL;
	act = malloc(sizeof(action_t));
	act->op = pmemoryEvent->action.op;
	act->page = pmemoryEvent->action.page;
//	memcpy(&item->action, &pmemoryEvent->action, sizeof(action_t));
//	item->action = &pmemoryEvent->action;
	item->action = act;
	item->pid = pmemoryEvent->pid;
	item->time = pmemoryEvent->time;
	item->next = NULL;
//	item->entry = &pmemoryEvent;
	printf("%6u PID %3u PushedToStack\n", systemTime, pmemoryEvent->pid);
	if (stack == NULL) {
		//First entry in stack
		stack = item;
		firstStackElement = item;
		lastStackElement = item;
	}
	else {
		lastStackElement->next = item;
		lastStackElement = item;
	}
}

Boolean IsInStack(unsigned pid) {
	memoryEventStackEntry *entry = firstStackElement;
	while (entry != NULL) {
		//memoryEvent_t *memoryEvent = entry->entry;
		if (entry->pid == pid) {
			printf("PID %3u FOUND AS %3u WITH ACTION %s\n", pid, entry->pid, entry->action->op);
			return TRUE;
		}
		if (entry == lastStackElement) break;
		entry = entry->next;
	}
	return FALSE;
}
