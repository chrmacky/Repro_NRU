/* Functions for control and administration of processes including  */
/* creation, destruction, blocked-control...						*/
/* for comments on the functions see the associated .h-file 		*/

/* ----------------------------------------------------------------	*/
/* Include required external definitions */
#include "processcontrol.h"

/* ----------------------------------------------------------------	*/
/* Declare global variables according to definition in globals.h	*/

/* ----------------------------------------------------------------	*/
/* Declarations of global variables visible only in this file 		*/
void resetPCB (PCB_t *pcb)
/* initilises a PCB with data representing an empty process entry	*/
{
	pcb->valid = FALSE; 		
	pcb->pid = 0; 
	pcb->ppid = 0; 
	pcb->ownerID = 0;
	pcb->start = 0; 
	pcb->duration = 0; 
	pcb->usedCPU = 0; 
	pcb->type = foreground; 
	pcb->status = init;
	// pcb->simInfo;   	// currently unused, 
						// placeholder, but is not initialised
	pcb->size = 0;		// process has no physical memory allocated
	pcb->pageTable = NULL;
}

/* ---------------------------------------------------------------- */
/* Functions for administration of processes in general 			*/
/* ---------------------------------------------------------------- */

Boolean initProcessTable(void)
/* allocates the process table and initialises it with empty entries		*/
{
	for (int i = 0; i <= MAX_PROCESSES; i++)
		resetPCB(&processTable[i]);
	return TRUE;
		
}

