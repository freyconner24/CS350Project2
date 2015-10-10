#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include "synchlist.h"
#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE 32

int CreateLock_sys(int vaddr) {
	kernelLock->Acquire();

	char* buffer = new char[BUFFER_SIZE + 1];
	buffer[BUFFER_SIZE] = '\0';

	copyin(vaddr, 1, buffer);

	userLocks[lockCount].userLock = new Lock(buffer);
	userLocks[lockCount].deleteFlag = FALSE;
	userLocks[lockCount].addrSpace = currentThread->space->getNumPages();

	int currentLockIndex = lockCount;
	lockCount++;

	kernelLock->Release();
	return currentLockIndex;
}

void Acquire_sys(int index) {

}

void Release_sys(int index) {

}

void DestroyLock_sys(int value) {

}
