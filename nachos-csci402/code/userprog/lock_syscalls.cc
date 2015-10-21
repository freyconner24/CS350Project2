#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include "synchlist.h"
#include "addrspace.h"
#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE 32

int CreateLock_sys(int vaddr, int size, int appendNum) {
	kernelLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode
	if (currentThread->space->maxLockCount == MAX_LOCK_COUNT){
		printf(currentThread->getName());
		printf(" has too many locks!-----------------------\n");
		return -1;
	}
	char* buffer = new char[size + 1];
	buffer[size] = '\0'; //end the char array with a null character

	if (copyin(vaddr, size, buffer) == -1){
		printf("%s","Bad pointer passed to to write: data not written\n");
		delete[] buffer;
		return -1;
	}; //copy contents of the virtual addr (ReadRegister(4)) to the buffer
	if (currentThread->space->lockCount < 0 || currentThread->space->lockCount >= MAX_LOCK_COUNT){
		printf("%s","ERROR IN LOCKCOUNT-----------------------\n");
		return -1;
	}
	currentThread->space->userLocks[currentThread->space->lockCount].userLock = new Lock(buffer); // instantiate new lock
	currentThread->space->userLocks[currentThread->space->lockCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	currentThread->space->userLocks[currentThread->space->lockCount].inUse = FALSE; // indicate the lock is not in use
	currentThread->space->userLocks[currentThread->space->lockCount].isDeleted = FALSE; // indicate the lock is not in use
	++(currentThread->space->maxLockCount);
	int currentLockIndex = currentThread->space->lockCount; // save the currentlockcount to be returned later
	++(currentThread->space->lockCount);
	DEBUG('a', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	DEBUG('l', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	printf("Lock  number %d  and name %s is created by %s \n", currentLockIndex, currentThread->space->userLocks[currentLockIndex].userLock->getName(), currentThread->getName());

	kernelLock->Release(); //release kernel lock
	return currentLockIndex;
}

void Acquire_sys(int index) {
	kernelLock->Acquire();
	if (index < 0 || index >= currentThread->space->maxLockCount){
		printf("acquiring invalid lock-----------------------\n");
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){
		printf("acquiring destroyed lock-----------------------\n");
		return;
	}
	if(currentThread->space->userLocks[index].inUse == TRUE){
		printf(" lock already in use-----------------------\n");
		return;
	}

	 // CL: acquire kernelLock so that no other thread is running on kernel mode
	DEBUG('a', "Lock  number %d and name %s\n", index, currentThread->space->userLocks[index].userLock->getName());
	printf("Lock  number %d  and name %s is acquired by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
	currentThread->space->userLocks[index].inUse = TRUE;
 //TODO: race condition?
	Lock* userLock = currentThread->space->userLocks[index].userLock;
	if(userLock->lockStatus != userLock->FREE) {
		updateProcessThreadCounts(currentThread->space, SLEEP);
	}
	currentThread->space->userLocks[index].userLock->Acquire(); // acquire userlock at index
	kernelLock->Release();//release kernel lock
}

void Release_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	if (index < 0 || index >= currentThread->space->maxLockCount){
		printf("releasing invalid lock-----------------------\n");
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){
		printf("releasing destroyed lock-----------------------\n");
		return;
	}
	if(currentThread->space->userLocks[index].inUse == FALSE){
		printf(" error: lock not in use-----------------------\n");
		return;
	}
	printf("Lock  number %d  and name %s is released by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
	currentThread->space->userLocks[index].inUse = FALSE;

	if(!currentThread->space->userLocks[index].userLock->waitQueueIsEmpty()) {
		updateProcessThreadCounts(currentThread->space, AWAKE);
	}
	currentThread->space->userLocks[index].userLock->Release(); // release userlock at index

	kernelLock->Release();//release kernel lock
}

void DestroyLock_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	if (index < 0 || index >= currentThread->space->maxLockCount){
		printf("destroying invalid lock-----------------------\n");
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){
		printf("destroying invalid lock-----------------------\n");
		return;
	}

	currentThread->space->userLocks[index].deleteFlag = TRUE;
	if (currentThread->space->userLocks[index].inUse == TRUE){
		printf("lock still in use-----------------------\n");
		return;
	}
	if (currentThread->space->userLocks[index].deleteFlag && !currentThread->space->userLocks[index].inUse){
		printf("Lock  number %d  and name %s is destroyed by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
		currentThread->space->userLocks[index].isDeleted = TRUE;
		delete currentThread->space->userLocks[index].userLock;
	 	--(currentThread->space->lockCount);
	}
	kernelLock->Release();//release kernel lock
}
