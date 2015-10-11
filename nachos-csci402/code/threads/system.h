// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "synchlist.h"
#include "stats.h"
#include "timer.h"

#define MAX_LOCK_COUNT 50
#define MAX_COND_COUNT 50
#define TEMP_ARRAY_SIZE 10000

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

struct UserLock {
	bool deleteFlag;
	bool inUse;
	Lock* userLock;
	AddrSpace* addrSpace;
};

struct UserCond {
	bool deleteFlag;
	bool inUse;
	Condition* userCond;
	AddrSpace* addrSpace;
};

//list of these
struct ThreadManager {
	// every time I fork
	// the forking thread is the parent of the new thread you are forking
	// create instance of struct every time I fork a thread
	Thread* parentThread; // check for this
	Thread* childthreads[TEMP_ARRAY_SIZE]; //childThread->space
	int childrenCount = 0;
};

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
extern struct UserLock userLocks[MAX_LOCK_COUNT];
extern struct UserCond userConds[MAX_COND_COUNT];
extern struct ThreadManager threadManagers[TEMP_ARRAY_SIZE];
extern Lock* kernelLock;
extern int lockCount;
extern int condCount;
extern int threadManagerCount;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
