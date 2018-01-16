/* Include-file defining elementary data types used by the 			*/
/* operating system */
#ifndef __BS_TYPES__
#define __BS_TYPES__

typedef enum { FALSE = 0, TRUE } Boolean;


/* data type for storing of process IDs		*/
typedef unsigned pid_t;

/* data type for the possible types of processes */
/* the process type determines the IO-characteristic */
typedef enum
{
	os, interactive, batch, background, foreground
} processType_t;

/* data type for the process-states used for scheduling and life-	*/
/* cycle manegament of the processes 								*/
typedef enum
{
	init, running, ready, blocked, ended

} status_t;

/* data type for the different events that cause the scheduler to	*/
/* become active */
typedef enum
{
	completed, io, quantumOver

} schedulingEvent_t;

/* data type for the simulation environment */
/* the information contained ion this struct are not available to the os */
typedef struct simInfo_struct
{
	unsigned IOready;	// simulation time when IO is complete, may be used in the future
} simInfo_t;


/* data type for a page table entry, the page table is an array of this element type*/
typedef struct pageTableEntry_struct
{
	Boolean present; 
	Boolean modified;
	Boolean referenced; 
	int frame;			// physical memory address, if present
	int swapLocation;	// if page is not present, this indicates it's location in secondary memory
						// as the content of the pages is not used in this simulation, it is unused



} pageTableEntry_t;

/* data type for the Process Control Block */
/* +++ this might need to be extended to support future features	+++ */
/* +++ like additional schedulers or advanced memory management		+++ */
typedef struct PCB_struct
{
	Boolean valid;
	pid_t pid;
	pid_t ppid;
	unsigned ownerID;
	unsigned start;
	unsigned duration;
	unsigned usedCPU;
	processType_t type;
	status_t status;
	simInfo_t simInfo;
	unsigned size;				// size of logical process memory in pages
	pageTableEntry_t *pageTable;
} PCB_t;

/* data type for the possible actions wtr. memory usage by a process		*/
/* This data type is used to trigger the respective action in the memory	*/
/* management system														*/
typedef enum
{
	start, end, read, write, allocate, deallocate, error
} operation_t;

/* data type for possible memory use by aprocess, kombining the action with	*/
/* the page that is used for this action, e.g. for reading */
/* is the action does not require a page number (i.e. a location in virtual	*/
/* memory), the value of 'page' is not used an may be undefined				*/		
typedef struct action_struct
{
	operation_t op;
	unsigned page;
} action_t;

/* data type for an event, descrtibing a cartain action performad by a		*/
/* process at agiven point in time.											*/
/* It is used for modelling the activities of a process during it's			*/
/* execution wrt. accessing the memory										*/
typedef struct event_struct
{
	unsigned time; 
	pid_t pid;
	action_t action;
} memoryEvent_t;

/* list type used by the OS to keep track of the currently available frames	*/ 
/* in physical memory. Used for allocating additional and freeing used		*/
/* pyhsical memory for/by processes											*/
typedef struct frameListEntry_struct
{
	int frame; 
	struct frameListEntry_struct *next;
} frameListEntry_t;

typedef frameListEntry_t* frameList_t;

#endif  /* __BS_TYPES__ */ 