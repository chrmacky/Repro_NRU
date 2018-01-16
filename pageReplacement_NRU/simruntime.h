/* Header file for the the simulation run-time environment.			*/
/* The functions in this file simulate the remainig system			*/
/* (covering hardware-related and software components)				*/
/* !!!!!!!!!! Do not change the functions in this file !!!!!!!!!!	*/

#ifndef __SIMRT__
#define __SIMRT__

extern Boolean simComplete;		// end of OS indicator
extern Boolean sim_randomAccess;		// flag for random access stimulus generation
/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include <math.h>
#include "bs_types.h"
#include "global.h"

typedef struct sim_frame_struct
{
	unsigned pid;			// zero indigating unused
	int page;				// negative values indigating unused
} sim_frame_t;


int sim_initSim(void);
/* initialise the simulation, not part of the os					*/

int sim_CloseSim(void);
/* Exit from the simulation environment regularly					*/

memoryEvent_t* sim_ReadNextEvent(memoryEvent_t* pMemoryEvent);
/* uses the file handle of the already opened stimulus file	"runFile"		*/
/* the pointer pMemoryEvent must point to a valid memoryEvent_t variable,	*/
/* the function does not allocate memory for this. 							*/
/* returns NULL if no action block was read, either because the file		*/
/* handle was invalid (NULL) or EOF was reached								*/
/* returns the pointer pMemoryEvent on success, containing the Action		*/
/* to perform	*/

memoryEvent_t* sim_NextRandomAccess(memoryEvent_t* pMemoryEvent);
/* generates a randomly generated memory event. From all existing Processes */
/* listed in processes.txt using the full logic memory of the process a page*/ 
/* is chosen randomly and a memory event (randomly read or write) generated.*/
/* Between two consecutive events lies a randomly chosen multiple of 5 units*/
/* This function requires all processes to have their page table initialised*/
/* The pointer pMemoryEvent must point to a valid memoryEvent_t variable,	*/
/* the function does not allocate memory for this. 							*/
/* Returns NULL if no memory event could be generated						*/
/* Returns the pointer pMemoryEvent on success, containing the Action		*/
/* to perform	*/

void sim_UpdateMemoryMapping(unsigned pid, action_t action, int frame);
/* keep track of use of the physical memory in the simulation				*/			
/* This is unly used for analysis and tracking of the OS-behaviour			*/
/* !!!!!!!   This mapping is not available to any part of the OS      !!!!!!*/


#endif /* __SIMRT__ */