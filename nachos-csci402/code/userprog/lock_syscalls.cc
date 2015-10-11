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

	copyin(vaddr, BUFFER_SIZE, buffer); //copy contents of the virtual addr (ReadRegister(4)) to the buffer
	if (lockCount < 0 || lockCount >= MAX_LOCK_COUNT){
		DEBUG('b',"ERROR IN LOCKCOUNT\n");
	}
	userLocks[lockCount].userLock = new Lock(buffer); // instantiate new lock
	userLocks[lockCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	userLocks[lockCount].addrSpace = currentThread->space; // copy address space from current thread to the lock, because the lock belongs to the current thread

	int currentLockIndex = lockCount; // save the currentlockcount to be returned later
	lockCount++;
	DEBUG('a', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	DEBUG('l', "Lock has number %d and name %s\n", currentLockIndex, buffer);
	printf("Lock  number %d  and name %s is created by %s \n", currentLockIndex, userLocks[currentLockIndex].userLock->getName(), currentThread->getName());

	kernelLock->Release(); //release kernel lock
	return currentLockIndex;
}

void Acquire_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	DEBUG('a', "Lock  number %d and name %s\n", index, userLocks[index].userLock->getName());
	printf("Lock  number %d  and name %s is acquired by %s \n", index, userLocks[index].userLock->getName(), currentThread->getName());
	userLocks[index].inUse = TRUE;
 //TODO: race condition?
	kernelLock->Release();//release kernel lock
	userLocks[index].userLock->Acquire(); // acquire userlock at index
}

void Release_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	printf("Lock  number %d  and name %s is released by %s \n", index, userLocks[index].userLock->getName(), currentThread->getName());
	userLocks[index].inUse = FALSE;

	kernelLock->Release();//release kernel lock
	userLocks[index].userLock->Release(); // release userlock at index
}

void DestroyLock_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	userLocks[index].deleteFlag = TRUE;
	if (userLocks[index].deleteFlag && !userLocks[index].inUse){
		printf("Lock  number %d  and name %s is destroyed by %s \n", index, userLocks[index].userLock->getName(), currentThread->getName());
		delete userLocks[index].userLock;
	}

	kernelLock->Release();//release kernel lock
}
