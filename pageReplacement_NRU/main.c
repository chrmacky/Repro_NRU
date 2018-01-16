// main() : Einstiegspunkt für die Anwendung.
//

#include <stdio.h>
#include <tchar.h>


#include "bs_types.h"
#include "global.h"
#include "core.h"


/* ----------------------------------------------------------------	*/
/* Declare global variables according to definition in global.h	*/
unsigned systemTime = 0; 		// the current system time (up time)
extern PCB_t processTable[]; 	// the process table

int main(int argc, char *argv[])
{	// starting point, all processing is done in called functions
	initOS();					// initialise operating system
	sim_initSim();				// initialise simulation run-time environment
	logGeneric("Starting Batch-run");
	coreLoop();					// start main loop of the OS
	logGeneric("Batch complete, shutting down");
	sim_CloseSim();				// shut down simulation envoronment
	fflush(stdout);				// make sure the output on the console is complete 
	return 1;
}