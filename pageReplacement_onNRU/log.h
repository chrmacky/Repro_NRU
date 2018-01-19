/* header-file defining the interface for logguig functions tothe screen	*/

#ifndef __LOG__
#define __LOG__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bs_types.h"
#include "global.h"


void logGeneric(char* message);
/* print the given general string to stdout and/or a log file 				*/
/* The time stamp (systemTime) will be prepended automatically				*/
/* the linefeed is automatically appended									*/

void logPid(unsigned pid, char* message);
/* print the given  string to stdout and/or a log file 						*/
/* The time stamp (systemTime) and the  PID are prepended automatically		*/
/* the linefeed is automatically appended									*/

void logPidMemAccess(unsigned pid, action_t action);
/* print the accessed page of the virtual address with PID to stdout		*/	

void logPidMemPhysical(unsigned pid, unsigned page, unsigned frame);
/* print the resolved pair of virtual address (page) to			 			*/
/* physical address (frame) with the PID at the current systemTime			*/

void logMemoryMapping(void); 
/* prints out a memory map showing the use of all frames of the physical mem*/

#endif /* __LOG__ */