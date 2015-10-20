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

	if (copyin(vaddr, BUFFER_SIZE, buffer) == -1){
		printf("%s","Bad pointer passed to to write: data not written\n");
		delete[] buffer;
		return -1;
	}; //copy contents of the virtual addr (ReadRegister(4)) to the buffer
	if (currentThread->space->lockCount < 0 || currentThread->space->lockCount >= MAX_LOCK_COUNT){
		printf("%s","ERROR IN LOCKCOUNT\n");
		interrupt->Halt();
	}
	currentThread->space->userLocks[currentThread->space->lockCount].userLock = new Lock(buffer); // instantiate new lock
	currentThread->space->userLocks[currentThread->space->lockCount].deleteFlag = FALSE; // indicate the lock is not to be deleted

	int currentLockIndex = currentThread->space->lockCount; // save the currentlockcount to be returned later
	++(currentThread->space->lockCount);
	DEBUG('a', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	DEBUG('l', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	printf("Lock  number %d  and name %s is created by %s \n", currentLockIndex, currentThread->space->userLocks[currentLockIndex].userLock->getName(), currentThread->getName());

	kernelLock->Release(); //release kernel lock
	return currentLockIndex;
}

void Acquire_sys(int index) {
	if (index < 0 || index > currentThread->space->lockCount-1){
		printf("acquiring invalid lock\n");
		return;
	}
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	DEBUG('a', "Lock  number %d and name %s\n", index, currentThread->space->userLocks[index].userLock->getName());
	printf("Lock  number %d  and name %s is acquired by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
	currentThread->space->userLocks[index].inUse = TRUE;
 //TODO: race condition?
	Lock* userLock = currentThread->space->userLocks[index].userLock;
	if(userLock->lockStatus != userLock->FREE) {
		updateProcessThreadCounts(currentThread->space, SLEEP);
	}
	kernelLock->Release();//release kernel lock
	currentThread->space->userLocks[index].userLock->Acquire(); // acquire userlock at index
}

void Release_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	printf("Lock  number %d  and name %s is released by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
	currentThread->space->userLocks[index].inUse = FALSE;

	if(!currentThread->space->userLocks[index].userLock->waitQueueIsEmpty()) {
		updateProcessThreadCounts(currentThread->space, AWAKE);
	}

	kernelLock->Release();//release kernel lock
	currentThread->space->userLocks[index].userLock->Release(); // release userlock at index
}

void DestroyLock_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	currentThread->space->userLocks[index].deleteFlag = TRUE;
	if (currentThread->space->userLocks[index].deleteFlag && !currentThread->space->userLocks[index].inUse){
		printf("Lock  number %d  and name %s is destroyed by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
		delete currentThread->space->userLocks[index].userLock;
	 	--(currentThread->space->lockCount);
	}

	kernelLock->Release();//release kernel lock
}
