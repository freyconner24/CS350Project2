#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include "synchlist.h"
#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE 32

int CreateLock_sys(int vaddr) {
	kernelLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode

	char* buffer = new char[BUFFER_SIZE + 1];
	buffer[BUFFER_SIZE] = '\0'; //end the char array with a null character

	copyin(vaddr, 1, buffer); //copy contents of the virtual addr (ReadRegister(4)) to the buffer
	if (lockCount < 0 || lockCount >= MAX_LOCK_COUNT){
		DEBUG('b',"ERROR IN LOCKCOUNT\n");
	}
	userLocks[lockCount].userLock = new Lock(buffer); // instantiate new lock
	userLocks[lockCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	userLocks[lockCount].addrSpace = currentThread->space; // copy address space from current thread to the lock, because the lock belongs to the current thread

	int currentLockIndex = lockCount; // save the currentlockcount to be returned later
	lockCount++;

	kernelLock->Release(); //release kernel lock
	return currentLockIndex;
}

void Acquire_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	userLocks[index].userLock->inUse = TRUE;

	kernelLock->Release();//release kernel lock
	userLocks[index].userLock->Acquire(); // acquire userlock at index
}

void Release_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	userLocks[index].userLock->inUse = FALSE;

	kernelLock->Release();//release kernel lock
	userLocks[index].userLock->Release(); // release userlock at index
}

void DestroyLock_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	userLocks[index].deleteFlag = TRUE;
	if (userLocks[index].deleteFlag && !userLocks[index].inUse){
		delete userLocks[index].userLock;
		userLocks[index] = NULL;
	}

	kernelLock->Release();//release kernel lock
}
