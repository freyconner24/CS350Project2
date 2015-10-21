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
	currentThread->space->locksLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode
	if (currentThread->space->lockCount >= MAX_LOCK_COUNT){
		printf(currentThread->getName());
		printf(" has too many locks!-----------------------\n");
		currentThread->space->locksLock->Release();
		return -1;
	}
	if (size < 0 || size >= 32){
		printf(currentThread->getName());
		printf(" Size must be between 0 and 32!-----------------------\n");
		currentThread->space->locksLock->Release();
		return -1;
	}
	char* buffer = new char[size + 1];
	buffer[size] = '\0'; //end the char array with a null character

	if (copyin(vaddr, size, buffer) == -1){
		printf("%s"," COPYIN FAILED\n");
		delete[] buffer;
		currentThread->space->locksLock->Release();
		return -1;
	}; //copy contents of the virtual addr (ReadRegister(4)) to the buffer

	currentThread->space->userLocks[currentThread->space->lockCount].userLock = new Lock(buffer); // instantiate new lock
	currentThread->space->userLocks[currentThread->space->lockCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	currentThread->space->userLocks[currentThread->space->lockCount].isDeleted = FALSE; // indicate the lock is not in use
	int currentLockIndex = currentThread->space->lockCount; // save the currentlockcount to be returned later
	++(currentThread->space->lockCount);
	DEBUG('a', " Lock has number %d and name %s\n", currentLockIndex, buffer);
	DEBUG('l', " Lock has number %d and name %s\n", currentLockIndex, buffer);
	printf(" Lock  number %d  and name %s is created by %s \n", currentLockIndex, currentThread->space->userLocks[currentLockIndex].userLock->getName(), currentThread->getName());
	currentThread->space->locksLock->Release(); //release kernel lock
	return currentLockIndex;
}

void Acquire_sys(int index) {
	currentThread->space->locksLock->Acquire();
	if (index < 0 || index >= currentThread->space->lockCount){
		printf(" Lock number %d invalid, thread %s can't acquire-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}

	if (currentThread->space->userLocks[index].isDeleted == TRUE){
		printf(" Lock number %d already destroyed, thread %s can't acquire-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		interrupt->Halt();
	}

	if(currentThread->space->userLocks[index].userLock->lockStatus == currentThread->space->userLocks[index].userLock->BUSY){
		printf(" Lock number %d and name %s already in use, adding to queue-----------------------\n", index, currentThread->space->userLocks[index].userLock->getName());
		currentThread->space->locksLock->Release();
		currentThread->space->userLocks[index].userLock->Acquire(); // acquire userlock at index
		return;
	}

	 // CL: acquire kernelLock so that no other thread is running on kernel mode
	DEBUG('a', "Lock  number %d and name %s\n", index, currentThread->space->userLocks[index].userLock->getName());
	printf(" Lock  number %d  and name %s is acquired by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
 //TODO: race condition?
	Lock* userLock = currentThread->space->userLocks[index].userLock;
	if(userLock->lockStatus != userLock->FREE) {
		updateProcessThreadCounts(currentThread->space, SLEEP);
	}
	currentThread->space->locksLock->Release();//release kernel lock
	currentThread->space->userLocks[index].userLock->Acquire(); // acquire userlock at index
}

void Release_sys(int index) {
	currentThread->space->locksLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	Lock* userLock = currentThread->space->userLocks[index].userLock;
	if (index < 0 || index >= currentThread->space->lockCount){
		printf(" Lock number %d invalid, thread %s can't release-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){
		printf(" Lock number %d already destroyed, %s can't release-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}
	if(userLock->lockStatus == userLock->FREE){
		printf(" lock not in use, nothing is done-----------------------\n");
		currentThread->space->locksLock->Release();
		return;
	}
	printf(" Lock number %d and name %s is released by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());

	if(!currentThread->space->userLocks[index].userLock->waitQueueIsEmpty()) {
		updateProcessThreadCounts(currentThread->space, AWAKE);
	}
	currentThread->space->locksLock->Release();//release kernel lock
	currentThread->space->userLocks[index].userLock->Release(); // release userlock at index

	if(currentThread->space->userLocks[index].userLock->lockStatus == currentThread->space->userLocks[index].userLock->BUSY) {
		printf(" Lock  number %d  and name %s is destroyed by %s \n", index, currentThread->space->userLocks[index].userLock->getName(), currentThread->getName());
		currentThread->space->userLocks[index].isDeleted = TRUE;
		delete currentThread->space->userLocks[index].userLock;	
	}
}

void DestroyLock_sys(int index) {
	currentThread->space->locksLock->Acquire();; // CL: acquire locksLock so that no other thread is running on kernel mode
	if (index < 0 || index >= currentThread->space->lockCount){ // check if lock index is valid
		printf(" Lock number %d invalid, thread %s can't destroy-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}
	if (currentThread->space->userLocks[index].isDeleted == TRUE){ // check if lock is already destroyed
		printf(" Lock number %d already destroyed, thread %s can't destroy-----------------------\n", index, currentThread->getName());
		currentThread->space->locksLock->Release();
		return;
	}

	currentThread->space->userLocks[index].deleteFlag = TRUE;
	if (currentThread->space->userLocks[index].userLock->lockStatus == currentThread->space->userLocks[index].userLock->BUSY){
		printf(" Lock number %d and name %s still in use-----------------------\n", index, currentThread->space->userLocks[index].userLock->getName());
	}
	currentThread->space->locksLock->Release();//release kernel lock
}
