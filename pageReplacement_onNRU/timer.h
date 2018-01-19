/* Include-file defining the interface of the timer event handlers			*/

#ifndef __TIMER__
#define __TIMER__

#include "bs_types.h"
#include "core.h"
#include "log.h"
#include "simruntime.h"

void timerEventHandler(void); 
/* The event Handler (aka ISR) of the timer event. 							*/
/* Will be triggered by the simulation environment periodically based on	*/
/* the period given by TIMER_INTERVAL (see global.h)						*/



/* ----------------------------------------------------------------	*/
/* Define global variables that will be visible in all sourcefiles	*/

#endif  /* __TIMER__ */ 