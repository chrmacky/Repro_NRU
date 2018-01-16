/* header-file defining the interface of the process contol					*/
/* the process control contains all functions required to administrate the 	*/
/* simulated processes 														*/

#ifndef __PROCESSCONTROL__
#define __PROCESSCONTROL__

#include "bs_types.h"
#include "global.h"

Boolean initProcessTable(void);		// const unsigned int maxPID
/* allocates the process table and initialises it with empty entries		*/

#endif /* __PROCESSCONTROL__ */
