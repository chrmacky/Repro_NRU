// pageReplacement.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//
/* Autor:  Tarik Gün und Christoph Lemke						*/

#include "memoryManagement.h"

unsigned emptyFrameCounter;		// number of empty Frames 
frameList_t emptyFrameList = NULL;
frameListEntry_t *emptyFrameListTail = NULL;
memoryFrameList_t clockFrameList = NULL;
memoryFrameListEntry_t *clockCurrentPointer = NULL;


/* ------------------------------------------------------------------------ */
/*		               Declarations of local helper functions				*/


Boolean isPagePresent(unsigned pid, unsigned page);
/* Predicate returning the present/absent status of the page in memory		*/

Boolean storeEmptyFrame(int frame);
/* Store the frame number in the data structure of empty frames				*/
/* and update emptyFrameCounter												*/

/* Clock page replacement algorithm											*/
int findNextFrame(void);


Boolean movePageOut(unsigned pid, unsigned page, int frame); 
/* Creates an empty frame at the given location.							*/
/* Copies the content of the frame occupid by the given page to secondary	*/
/* storage.																	*/ 
/* The exact location in seocondary storage is found and alocated by		*/ 
/* this function.															*/
/* The page table entries are updated to reflect that the page is no longer */
/* present in RAM, including its location in seondary storage				*/
/* Returns TRUE on success and FALSE on any error							*/

Boolean movePageIn(unsigned pid, unsigned page, unsigned frame);
/* Returns TRUE on success and FALSE on any error							*/

Boolean updatePageEntry(unsigned pid, action_t action);
/* updates the data relevant for page replacement in the page table entry,	*/
/* e.g. set reference and modyfy bit.										*/
/* In this simulation this function has to cover also the required actions	*/
/* nornally done by hardware, i.e. it summarises the actions of MMu and OS  */
/* when accessing physical memory.											*/
/* Returns TRUE on success and FALSE on any error							*/


/* ------------------------------------------------------------------------ */
/*                Start of public Implementations							*/

// fill clock with x amount of empty entries depending on the size of "MEMORYSIZE"
Boolean initMemoryManager(void)
{
	memoryFrameListEntry_t* current = NULL;
	// mark all frames of the physical memory as empty 
	for (int i = 0; i < MEMORYSIZE; i++) {

		current = malloc(sizeof(memoryFrameListEntry_t));
		current->frame = i;
		current->referenced = FALSE;
		if (clockFrameList == NULL)
		{
			clockFrameList = current;
			clockCurrentPointer = current;
		}
		else {
			clockCurrentPointer->next = current;
			clockCurrentPointer = current;
		}
		storeEmptyFrame(i);
	}

	clockCurrentPointer->next = clockFrameList;
	clockCurrentPointer = clockFrameList;

	return TRUE;
}


// Iterate through clock and set reference attribute of each element to FALSE
Boolean cleanupClockMemory() {

	for (int i = 0; i < MEMORYSIZE; i++)
		clockFrameList[i].referenced = FALSE;

	return TRUE;
}

int accessPage(unsigned pid, action_t action)
/* handles the mapping from logical to physical address, i.e. performs the	*/
/* task of the MMU and parts of the OS in a computer system					*/
/* Returns the number of the frame on success, also in case of a page fault */
/* Returns a negative value on error										*/
{
	int frame = INT_MAX;		// the frame the page resides in on return of the function
	unsigned outPid = pid;
	unsigned outPage= action.page;
	// check if page is present
	if (isPagePresent(pid, action.page))
	{// yes: page is present
		// look up frame in page table and we are done
		frame = processTable[pid].pageTable[action.page].frame;
	}
	else
	{// no: page is not present
		logPid(pid, "Pagefault");

		// pagefault, set frame to return value of "findNextFrame"
		// running replacement algorithm 
		frame = findNextFrame();
		// move page in to empty frame
		movePageIn(pid, action.page, frame);
	}
	if (frame >= 0)
		clockFrameList[frame].referenced = TRUE;
	// update page table for replacement algorithm
	updatePageEntry(pid, action);
	return frame;
}

Boolean createPageTable(unsigned pid)
/* Create and initialise the page table	for the given process									*/
/* Information on max. process size must be already stored in the PCB		*/
/* Returns TRUE on success, FALSE otherwise									*/
{
	pageTableEntry_t *pTable = NULL;
	// create and initialise the page table of the process
	pTable = malloc(processTable[pid].size * sizeof(pageTableEntry_t));
	if (pTable == NULL) return FALSE; 
	// initialise the page table
	for (unsigned i = 0; i < processTable[pid].size; i++)
	{
		pTable[i].present = FALSE;
		pTable[i].frame = -1;
		pTable[i].swapLocation = -1;
	}
	processTable[pid].pageTable = pTable; 
	return TRUE;
}

Boolean deAllocateProcess(unsigned pid)
/* free the physical memory used by a process, destroy the page table		*/
/* returns TRUE on success, FALSE on error									*/
{
	// iterate the page table and mark all currently used frames as free
	pageTableEntry_t *pTable = processTable[pid].pageTable;
	for (unsigned i = 0; i < processTable[pid].size; i++)
	{
		if (pTable[i].present == TRUE)
		{	// page is in memory, so free the allocated frame
			clockFrameList[pTable[i].frame].referenced = FALSE;
			storeEmptyFrame(pTable[i].frame);	// add to pool of empty frames
			// update the simulation accordingly !! DO NOT REMOVE !!
			sim_UpdateMemoryMapping(pid, (action_t) { deallocate, i }, pTable[i].frame);
		}
	}
	free(processTable[pid].pageTable);	// free the memory of the page table
	return TRUE;
}

/* ---------------------------------------------------------------- */
/*                Implementation of local helper functions          */

Boolean isPagePresent(unsigned pid, unsigned page)
/* Predicate returning the present/absent status of the page in memory		*/
{
	return processTable[pid].pageTable[page].present; 
}


Boolean storeEmptyFrame(int frame)
/* Store the frame number in the data structure of empty frames				*/
/* and update emptyFrameCounter												*/
{
	frameListEntry_t *newEntry = NULL;
	newEntry = malloc(sizeof(frameListEntry_t)); 
	if (newEntry != NULL)
	{
		// create new entry for the frame passed
		newEntry->next = NULL;			
		newEntry->frame = frame;
		if (emptyFrameList == NULL)			// first entry in the list
		{
			emptyFrameList = newEntry;
		}
		else								// appent do the list
			emptyFrameListTail->next = newEntry;
		emptyFrameListTail = newEntry; 
		emptyFrameCounter++;				// one more free frame
	}
	clockFrameList[frame].referenced = FALSE;
	return (newEntry != NULL);
}

// runs the clock page replacement algorithm
int findNextFrame(void) {
	int frame = -1;
	// Run frame found and returned
	while (TRUE) {
		// Do the actual replacement if a non referenced frame is found
		if (!clockFrameList[clockCurrentPointer->frame].referenced) {
			frame = clockCurrentPointer->frame;
			clockCurrentPointer = clockCurrentPointer->next;

			//check, if this frame was actually used by any process and move this frame out of memory
			for (unsigned pid = 0; pid < MAX_PROCESSES; pid++) {
				if (processTable[pid].valid && processTable[pid].pageTable != NULL) {
					for (unsigned page = 0; page < processTable[pid].size; page++) {
						// Throw page out of physical memory and set attributes to FALSE if frame found
						if (processTable[pid].pageTable[page].frame == frame) {
							processTable[pid].pageTable[page].present = FALSE;
							processTable[pid].pageTable[page].referenced = FALSE;
							processTable[pid].pageTable[page].frame = -1;
							return frame;
						}
					}
				}
			}

			return frame;
		}
		// Set current frame reference to FALSE and move pointer
		clockCurrentPointer->referenced = FALSE;
		clockFrameList[clockCurrentPointer->frame].referenced = FALSE;
		clockCurrentPointer = clockCurrentPointer->next;
	}
}

// retunrs the value of the referenced bit on the specified frame
Boolean getFrameRBitState(int frame) {
	return clockFrameList[frame].referenced;
}
// returns TRUE or FALSE if the passed frame is exactly the frame 
// that is in the clockCurrentPointer list 
Boolean hasFrameClockPointer(int frame) {
	if (clockCurrentPointer->frame == frame) return TRUE;
	return FALSE;
}



Boolean movePageIn(unsigned pid, unsigned page, unsigned frame)
/* Returns TRUE on success ans FALSE on any error							*/
{
	// copy of the content of the page from secondary memory to RAM not simulated
	// update the page table: mark present, store frame number, clear statistics
	// *** This must be extended for advences page replacement algorithms ***
	processTable[pid].pageTable[page].frame = frame;
	processTable[pid].pageTable[page].present = TRUE;
	// page was just moved in, reset all statistics on usage, R- and P-bit at least
	processTable[pid].pageTable[page].modified = FALSE;
	processTable[pid].pageTable[page].referenced = FALSE;
	// Statistics for advanced replacement algorithms need to be reset here also

	// update the simulation accordingly !! DO NOT REMOVE !!
	sim_UpdateMemoryMapping(pid, (action_t) { allocate, page }, frame);
	return TRUE;
}

Boolean movePageOut(unsigned pid, unsigned page, int frame)
/* Creates an empty frame at the given location.							*/
/* Copies the content of the frame occupid by the given page to secondary	*/
/* storage.																	*/
/* The exact location in seocondary storage is found and alocated by		*/
/* this function.															*/
/* The page table entries are updated to reflect that the page is no longer */
/* present in RAM, including its location in seondary storage				*/
/* Returns TRUE on success and FALSE on any error							*/
{
	// allocation of secondary memory storage location and copy of page are ommitted for this simulation
	// no distinction between clean and dirty pages made at this point
	// update the page table: mark absent, add frame to pool of empty frames
	// *** This must be extended for advences page replacement algorithms ***
	processTable[pid].pageTable[page].present = FALSE;	

	storeEmptyFrame(frame);	// add to pool of empty frames
	// update the simulation accordingly !! DO NOT REMOVE !!
	sim_UpdateMemoryMapping(pid, (action_t) { deallocate, page }, frame);
	return TRUE;
}

Boolean updatePageEntry(unsigned pid, action_t action)
/* updates the data relevant for page replacement in the page table entry,	*/
/* e.g. set reference and modify bit.										*/
/* In this simulation this function has to cover also the required actions	*/
/* nornally done by hardware, i.e. it summarises the actions of MMU and OS  */
/* when accessing physical memory.											*/
/* Returns TRUE on success ans FALSE on any error							*/
// *** This must be extended for advences page replacement algorithms ***
{
	processTable[pid].pageTable[action.page].referenced = TRUE; 
	if (action.op == write)
		processTable[pid].pageTable[action.page].modified = TRUE;
	return TRUE; 
}