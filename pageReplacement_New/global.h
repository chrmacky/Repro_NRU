/* Include-file defining global defines and constants  	 		*/
/* used by the operating system */
#ifndef __GLOBAL__
#define __GLOBAL__

#define	_CRT_SECURE_NO_WARNINGS		// suppress legacy warnings 

#include <stdlib.h>
#include <time.h>
#include "bs_types.h"
#include "core.h"
#include "memoryManagement.h"
#include "processcontrol.h"
#include "log.h"
#include "simruntime.h"
#include "timer.h"


// Number of possible concurrent processes, i.e. size of the process table 
#define MAX_PROCESSES (unsigned)100

// Size of the physical memory available to user processes in frames
#define MEMORYSIZE 4

// Period of the timer. on all multiples of this value the timer ISR ist called by the simulation
#define TIMER_INTERVAL 50

// name of the file with process definitions
#define PROCESS_FILENAME "processes.txt"
// name of the file with the simulation run an empty file name switches to random event stimulus
#define RUN_FILENAME "run.txt"
//#define RUN_FILENAME ""

/* ----------------------------------------------------------------	*/
/* Define global variables that will be visible in all sourcefiles	*/
extern unsigned int	maxPID;				// largest valid PID
PCB_t processTable[MAX_PROCESSES + 1]; 	// the process table
extern unsigned systemTime; 			// the current system time (up time)

/* ----------------------------------------------------------------	*/
/* Define global constants that will be visible in all sourcefiles	*/


#endif  /* __GLOBAL__ */ 