/* Implementation of the simulation run-time environment.			*/
/* The functions in this file simulate the remainig system			*/
/* (covering hardware-related and software components)				*/
/* for comments on the functions see the associated .h-file			*/
/* !!!!!!!!!! Do not change the functions in this file !!!!!!!!!!	*/

#define	_CRT_SECURE_NO_WARNINGS		// suppress legacy warnings 

/* ---------------------------------------------------------------- */
/* Include required external definitions */
#include <math.h>
#include "bs_types.h"
#include "global.h"

sim_frame_t sim_MemoryMap[MEMORYSIZE];	// Array storing the use of physical memory. For simulation use ONLY!

typedef struct sim_pidList_struct
{
	unsigned pid;
	struct sim_pidList_struct *next;
} sim_pidList_t;

Boolean stimulusComplete = FALSE;		// stimulus file completely read ?
Boolean noMoreProcessesAvailable = FALSE;
Boolean simComplete = FALSE;		// end of OS indicator
FILE* runFile=NULL;					// the file containing the stimulus informatio
memoryEvent_t currentEvent;			// buffer for the next currently processed event
memoryEvent_t *pCurrentEvent;		// pointer to next event to process, NULL indicates none available
unsigned sim_processCount = 0;		// number of processes listed in process.txt
Boolean sim_randomAccess;			// flag for random access stimulus generation
// list of valid pid, i.e. processes used in the simulation
sim_pidList_t *sim_pidList = NULL, *sim_pidListTail = NULL;	

/* ------------------------------------------------------------------------ */
/*		               Declarations of local helper functions				*/

Boolean readProcessFile(const char* filename);
/* opens the  process info file specified by <filename>						*/
/* file has to refer to a valid FILE handle in the calling instance			*/
/* returns the file handle (which is NULL on error)							*/
/* Major side-effect is the creation and setting-up of the process table	*/
/* Data in the file must be read using the function sim_ReadNextEvent()		*/

FILE* openStimulusFile(FILE *file, const char* filename);
/* opens the file with the stimulus information specified by <filename>		*/
/* file has to refer to a valid FILE handle in the calling instance			*/
/* returns the file handle (which is NULL on error)							*/
/* Data in the file must be read using the function readNextAction()		*/

Boolean lineIsComment(const char* line);
/* predicat that return TRUE if the given string starts with '//'			*/
/* and FALSE otherwise */
/* Caution: No skipping of leading white-spaces								*/

Boolean addToSimProcesslist(unsigned pid);
/* append to the list of valid pids. Used for stimuls generation in the		*/	
/* simulation environment only												*/			

unsigned getNthPid(unsigned n); 
/* returns the Nth pid in the list of valid pids. Used for random access	*/
/* stimulus  */

/* ------------------------------------------------------------------------ */
/*                Start of public Implementations							*/

int sim_initSim(void)
/* initialise the simulation, not part of the os					*/
{
	char filename[128] = RUN_FILENAME;
	unsigned pid = 0; 

	systemTime = 0;				// reset the system time to zero
	// open the file with process definitions
	readProcessFile(PROCESS_FILENAME);
	if (strlen(filename) > 0)		// stimulus based on a file
	{	
		// open the file with stimulus information
		runFile = openStimulusFile(runFile, filename);
		if (runFile == NULL) exit(-1);
		sim_randomAccess = FALSE;
		logGeneric("Sim: Stimulus file opened");
	}
	else						// randon stimulus
	{
		sim_randomAccess = TRUE;
		// initialise all processes with empty page table
		for (unsigned i=1; i<=sim_processCount; i++ )
		{ 
			createPageTable(getNthPid(i)); 
		}
		logGeneric("Sim: Starting random access stimulus");
	}
	
	// init the internal log of the memory use of the simulation
	for (int i = 0; i < MEMORYSIZE; i++)
	{
		sim_MemoryMap[i].pid = 0;
		sim_MemoryMap[i].page = -1;
	}

	srand((unsigned int)time(NULL));

	stimulusComplete = FALSE;
	noMoreProcessesAvailable = FALSE; // there are still Actions

	return TRUE;
}

int sim_CloseSim(void)
/* Exit from the simulation environment regularly					*/
{
	sim_pidList_t *pDelete = NULL;
	// clear up list of valid processes 
	while (sim_processCount>0)
	{
		pDelete= sim_pidList; 
		sim_pidList = sim_pidList->next; 
		free(pDelete); 
		sim_processCount--;
	} 
	return TRUE;
}


memoryEvent_t* sim_ReadNextEvent(memoryEvent_t* pMemoryEvent)
/* Depending on the flag sim_randomAccess the next memory access event is	*/
/* created either based on the stimulus file: read the stimulus file and	*/
/* return the next sumlation event due for execution.						*/
/* Or, if no stimulus file was given, the generation of memory access events*/
/* is based on selection of the pid and a valid page number of that process */
// TODO : Liste von Ereignissen unterstützen zu einem Zeitpunkt 
{
	char linebuffer[129] = "";		// read buffer for file-input
	char eventTypeStr[21] = ""; 	// buffers for reading process type-string

	// array for possible periods to advance the simulation time
	unsigned simTimeDelta[12] = { 0,0,0,0,5,5,5,10,10,10,15,25 };	// for random stimulus
	unsigned myRandom,pid;											// for random stimulus

	if (sim_randomAccess == FALSE)				// file-based stimulus
	{
		if (runFile == NULL) return NULL;		// error: file handle not initialised
		if (feof(runFile)) {
			fclose(runFile);			// close the file on reaching EOF
			stimulusComplete = TRUE; // file completely processed
			return NULL;		// error occured (EOF reached)
		}
		else
		{	// read line, skip comment lines
			do {
				if (!feof(runFile))
					fgets(linebuffer, 128, runFile);
			} while ((!feof(runFile)) && (lineIsComment(linebuffer)));
			if (strcmp(linebuffer, "") == 0) {
				logGeneric("Error reading process-info file: empty line");
				return NULL;			// error occured: line is empty
			}
			else {
				// initialise with "error", if parsing of the event is OK, this will be overwritten
				pMemoryEvent->action.op = error; pMemoryEvent->action.page = 0;
				// printf("%6u : Sim: Read from File: %s", systemTime, linebuffer);
				sscanf(linebuffer, "%u %u %s", &pMemoryEvent->time, &pMemoryEvent->pid, eventTypeStr);
				// evaluate string, convert page to integer
				if (strcmp(eventTypeStr, "S") == 0)
					pMemoryEvent->action.op = start;
				else if (strcmp(eventTypeStr, "E") == 0)
					pMemoryEvent->action.op = end;
				else
					if (strlen(eventTypeStr) > 0)
					{
						sscanf(&eventTypeStr[1], "%i", &pMemoryEvent->action.page);
						if (eventTypeStr[0] == 'R') pMemoryEvent->action.op = read;
						else if (eventTypeStr[0] == 'W') pMemoryEvent->action.op = write;
						else pMemoryEvent->action.op = error;
					}
			}
		}
	}
	else						// random access stimulus
	{
		// create simulation time delta 
		pMemoryEvent->time = systemTime + simTimeDelta[rand() % 12];
		// choose pid (random index, lookup)
		myRandom = (rand() % sim_processCount) + 1;
		pid = getNthPid(myRandom);
		pMemoryEvent->pid = pid; 
		// select page
		pMemoryEvent->action.page = rand() % processTable[pid].size; 
		// choose r/w (3:1)
		myRandom = rand() % 4; 
		if (myRandom<3)
			pMemoryEvent->action.op = read; 
		else 
			pMemoryEvent->action.op = write;
	}
	return pMemoryEvent;
}

void sim_UpdateMemoryMapping(unsigned pid, action_t action, int frame)
/* keep track of use of the physical memory in the simulation				*/
/* This is unly used for analysis and tracking of the OS-behaviour			*/
/* !!!!!!!   This mapping is not available to any part of the OS      !!!!!!*/
{
	switch (action.op)
	{
	case allocate:
	case read:
	case write:
		sim_MemoryMap[frame].pid = pid;	// 
		sim_MemoryMap[frame].page = action.page; 
		break; 
	case deallocate:		// de-allocation
		sim_MemoryMap[frame].pid = 0;	// no process owns this frame
		sim_MemoryMap[frame].page = -1;	// no valid page in frame
	default: 
		break; 
	}
}

/* ---------------------------------------------------------------- */
/*                Implementation of local helper functions          */

Boolean readProcessFile(const char * filename)
/* reads the process informations by reading the given file twice			*/
/* The first iteration is used to determine the largest PID used which is	*/
/* used to determine the size of the process table.							*/
/* the process table is then initilalised with empty PCBs					*/
/* Based on the known size of the processtable, in the second iteration,the */
/* file is read and the information about all listed processes is read and	*/
/* stored in the process table initialised*/
/* Returns FALSE on any error, e.g. missing file or syntax errors			*/
{
	FILE* processFile;
	char linebuffer[257] = "x";			// read buffer for file-input
	unsigned int maxPID = MAX_PROCESSES;
	unsigned pid, size;					

	processFile = fopen(filename, "r");		// try to open file for read

	if (processFile == NULL)				// test for success and error handling
	{
		logGeneric("Error opening file containing process information");
		return FALSE;
	}

	// skip leading comment lines
	do {
		if (!feof(processFile))
			fgets(linebuffer, 256, processFile);
	} while ((!feof(processFile)) && (lineIsComment(linebuffer)));
	// now read information on all processes used for simulation
	do {
		// process current line
		sscanf(linebuffer, "%u %u", &pid, &size); 
		processTable[pid].size = size; 
		processTable[pid].valid = TRUE; 
		// printf("PID: %2u has %2u pages\n", pid, size);			// Debug file IO
		addToSimProcesslist(pid);		// store pid in list of valid pids for simulation!
		// read next line (or EOF) and skip comment lines
		do {
			if (!feof(processFile))
				fgets(linebuffer, 256, processFile);
		} while ((!feof(processFile)) && (lineIsComment(linebuffer)));
	} while (!feof(processFile));

	return TRUE;
}

FILE* openStimulusFile(FILE *file, const char * filename)
{

	char linebuffer[257] = "x";			// read buffer for file-input
	if (strlen(filename)>0)				// filename given --> file-based stimulus
	{
	file = fopen(filename, "r");		// try to open file for read
	// test for success and error handling
	if (file == NULL)
	{
		logGeneric("Error opening stimulus file");
		return NULL;
	}
	// skip first line, only a comment
	if (!feof(file))
		fgets(linebuffer, 128, file);
	printf("Read from File: %s", linebuffer);
	sim_randomAccess = FALSE; 
	}
	else							// no filename given --> random access stimulus
	{
		sim_randomAccess = TRUE;
	}
	noMoreProcessesAvailable = FALSE;	// the file was just opened, there should be some processes
	return file;
}

Boolean lineIsComment(const char* line)
{	// detects comments and empty lines
	if (line == NULL) return FALSE;	// error handling
	if (line[0] == '\0') return FALSE;
	if ((line[0] == '#') || (line[0] == '\n'))
		return TRUE;			// line marked as line-comment
	else
		return FALSE;
}

Boolean addToSimProcesslist(unsigned pid)
/* append to the list of valid pids. Used for stimuls generation in the		*/
/* simulation environment only												*/
{
	sim_pidList_t *newEntry = NULL;
	newEntry = malloc(sizeof(sim_pidList_t));
	if (newEntry != NULL)
	{
		newEntry->next = NULL;
		newEntry->pid= pid;
		if (sim_pidList == NULL)
		{
			sim_pidList = newEntry;
		}
		else
			sim_pidListTail->next = newEntry;
		sim_pidListTail = newEntry;
		sim_processCount++;		// one more valid pid
	}
	return (newEntry != NULL);
}


unsigned getNthPid(unsigned n)
/* returns the Nth pid in the list of valid pids. Counting starts with 1	*/ 
/* Used for random access stimulus  */
{
	sim_pidList_t *pList= sim_pidList;	//start from head
	unsigned pid = 0; 
	for (unsigned i = 1; i < n; i++)
	{
		pList = pList->next; 
		if (pList == NULL) break;
	}
	if (pList != NULL) pid = pList->pid; 
	return pid;
}
