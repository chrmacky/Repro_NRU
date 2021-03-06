/* Include-file defining the interface of the memory management system		*/
/* Autor:  Tarik G�n und Christoph Lemke						*/
#ifndef __MEMORY_MANAGEMENT__
#define __MEMORY_MANAGEMENT__

#include "bs_types.h"
#include "global.h"
#include "core.h"
#include "log.h"
#include "simruntime.h"

Boolean initMemoryManager(void);		// initialise the memory management system emptyFrameCounter = MEMSIZE;		
/* initialises the memory manager, allocates and iniatlises the				*/
/* required data structures													*/


/* set all frames reference attribute to FALSE								*/		
Boolean cleanupClockMemory();

/* Helper function for debug output,										*/
/* it should be displayed on all the frame when the R-BIT is set			*/
/* is for debug purposes to use.											*/
/* in the print output, it appears on every frame whether					*/
/* the R-Bit is set or not.													*/
Boolean getFrameRBitState(int frame); 

/* Helper function, that returns TRUE or FALSE if the pointer is on			*/
/* the passed frame															*/
Boolean hasFrameClockPointer(int frame);

int accessPage(unsigned pid, action_t action);
/* handles the mapping from logical to physical address, i.e. performs the	*/
/* task of the MMU in a computer system										*/
/* Returns the number of the frame on success, also in case of a page fault,*/
/* i.e. the handling of a page fault is covered by this function, except	*/
/* choosing a page to evict. The page replacement is only triggered by this */
/* function, but handled in a separate function								*/
/* Returns the number of the frame, the page resides in, and				*/
/* a negative value on error												*/

Boolean createPageTable(unsigned pid);
/* Create and initialise the page table	of the giveb process				*/
/* Information on max. process size must be already stored in the PCB		*/
/* Returns TRUE on success, FALSE otherwise									*/

Boolean deAllocateProcess(unsigned pid);
/* free the physical memory used by a process, destroy the page table		*/
/* returns TRUE on success, FALSE on error									*/


#endif  /* __MEMORY_MANAGEMENT__ */ 