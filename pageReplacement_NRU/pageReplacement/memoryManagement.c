// pageReplacement.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "memoryManagement.h"

unsigned emptyFrameCounter;		// number of empty Frames 
frameList_t emptyFrameList = NULL;
frameListEntry_t *emptyFrameListTail = NULL;

/* ------------------------------------------------------------------------ */
/*		               Declarations of local helper functions				*/


Boolean isPagePresent(unsigned pid, unsigned page);
/* Predicate returning the present/absent status of the page in memory		*/

Boolean storeEmptyFrame(int frame);
/* Store the frame number in the data structure of empty frames				*/
/* and update emptyFrameCounter												*/

int getEmptyFrame(void);
/* Returns the frame number of an empty frame.								*/
/* A return value of -1 indicates that no empty frame exists. In this case	*/
/* a page replacement algorithm must be called to free evict a page and		*/
/* thus clear one frame */

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

Boolean pageReplacement(unsigned *pid, unsigned *page, int *frame);
/* ===== The page replacement algorithm								======	*/
/* In the initial implementation the frame to be cleared is chosen			*/
/* globaly and randomly, i.e. a frame is chosen at random regardless of the	*/
/* process that is currently usigt it.										*/
/* The values of pid and page number passed to the function may be used by  */
/* local replacement strategies */
/* OUTPUT: */
/* The frame number, the process ID and the page currently assigned to the	*/
/* frame that was chosen by this function as the candidate that is to be	*/
/* moved out and replaced by another page is returned via the call-by-		*/
/* reference parameters.													*/
/* Returns TRUE on success and FALSE on any error							*/


/* ------------------------------------------------------------------------ */
/*                Start of public Implementations							*/


Boolean initMemoryManager(void)
{
	// mark all frames of the physical memory as empty 
	for (int i = 0; i < MEMORYSIZE; i++)
		storeEmptyFrame(i);
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
		// check for an empty frame
		frame = getEmptyFrame();
		if (frame < 0)
		{	// no empty frame available: start replacement algorithm to find candidate frame
			logPid(pid, "No empty frame found, running replacement algorithm");
			pageReplacement(&outPid, &outPage, &frame);
			// move candidate frame out to secondary storage
			movePageOut(outPid, outPage, frame);			
			frame = getEmptyFrame();
		} // now we have an empty frame to move the page into
		// move page in to empty frame
		movePageIn(pid, action.page, frame);
	}
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
	return (newEntry != NULL); 
}

int getEmptyFrame(void)
/* Returns the frame number of an empty frame.								*/
/* A return value of -1 indicates that no empty frame exists. In this case	*/
/* a page replacement algorithm must be called to evict a page and thus 	*/
/* clear one frame															*/
{
	frameListEntry_t *toBeDeleted = NULL;
	int emptyFrameNo = -1;
	if (emptyFrameList == NULL) return -1;	// no empty frame exists
	emptyFrameNo = emptyFrameList->frame;	// get number of empty frame
	// remove entry of that frame from the list
	toBeDeleted = emptyFrameList;			
	emptyFrameList = emptyFrameList->next; 
	free(toBeDeleted); 
	emptyFrameCounter--;					// one empty frame less
	return emptyFrameNo; 
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


Boolean pageReplacement(unsigned *outPid, unsigned *outPage, int *outFrame)
/* ===== The page replacement algorithm								======	*/
/* In the initial implementation the frame to be cleared is chosen			*/
/* globaly and randomly, i.e. a frame is chosen at random regardless of the	*/
/* process that is currently usigt it.										*/
/* The values of pid and page number passed to the function may be used by  */
/* local replacement strategies */
/* OUTPUT: */
/* The frame number, the process ID and the page currently assigned to the	*/
/* frame that was chosen by this function as the candidate that is to be	*/
/* moved out and replaced by another page is returned via the call-by-		*/
/* reference parameters.													*/
/* Returns TRUE on success and FALSE on any error							*/
{
	Boolean found = FALSE;		// flag to indicate success
	// just for readbility local copies ot the passed values are used:
	unsigned pid = (*outPid); 
	unsigned page = (*outPage);
	int frame = *outFrame; 
	
	// +++++ START OF REPLACEMENT ALGORITHM IMPLEMENTATION: GLOBAL RANDOM ++++
	frame = rand() % MEMORYSIZE;		// chose a frame by random
	// As the initial implemetation does not have data structures that allows
	// easy retrieval of the identity of a page residing in a given frame, 
	// now the frame ist searched for in all page tables of all running processes
	// I.e.: Iterate through the process table and for each valid PCB check 
	//the valid entries in its page table until the frame is found
	pid = 0; page = 0; 
	do 
	{
		pid++;
		if ((processTable[pid].valid) && (processTable[pid].pageTable != NULL))
			for (page = 0; page < processTable[pid].size; page++)
				if (processTable[pid].pageTable[page].frame == frame) {
					found = TRUE;
					break;
				}
	} while ((!found) && (pid < MAX_PROCESSES));
	// +++++ END OF REPLACEMENT ALFGORITHM found indicates success/failure
	// RESULT is pid, page, frame

	// prepare returning the resolved location for replacement
	if (found)
	{	// assign the current values to the call-by-reference parameters
		(*outPid) = pid;
		(*outPage) = page; 
		(*outFrame) = frame;
	}
	return found; 
}