#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include "addrspace.h"
#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE 32

int CreateCondition_sys(int vaddr, int size, int appendNum) {
	currentThread->space->condsLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode
	if (currentThread->space->condCount == MAX_COND_COUNT){
		printf(currentThread->getName());
		printf(" has too many conditions!\n");
		currentThread->space->condsLock->Release();
		return -1;
	}

	if(size < 0 || size > 32) {
		printf("Size must be: (size < 0 || size > 32) ? %d\n", size);
		currentThread->space->condsLock->Release();
		return -1;
	}

	char* buffer = new char[BUFFER_SIZE + 1];
	buffer[BUFFER_SIZE] = '\0'; //end the char array with a null character

	if(copyin(vaddr, BUFFER_SIZE, buffer) == -1) { //copy contents of the virtual addr (ReadRegister(4)) to the buffer
		printf("copyin failed\n");
		delete [] buffer;
		currentThread->space->condsLock->Release();
		return -1;
	}
	if (currentThread->space->condCount < 0 || currentThread->space->condCount >= MAX_COND_COUNT){
		DEBUG('b',"ERROR IN CONDCOUNT\n");
		printf("condCount is neg or greater than max count: %d\n", currentThread->space->condCount);
		currentThread->space->condsLock->Release();
		return -1;
	}

	currentThread->space->userConds[currentThread->space->condCount].userCond = new Condition(buffer); // instantiate new lock
	currentThread->space->userConds[currentThread->space->condCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	// currentThread->space->userConds[currentThread->space->condCount].inUse = FALSE; // indicate the lock is not in use
	currentThread->space->userConds[currentThread->space->condCount].isDeleted = FALSE; // indicate the lock is not in use
	int currentCondIndex = currentThread->space->condCount; // save the currentlockcount to be returned later
	currentThread->space->condCount++;
	printf("Condition number %d and name %s is created by %s\n", currentCondIndex, currentThread->space->userConds[currentCondIndex].userCond->getName(), currentThread->getName());

	currentThread->space->condsLock->Release(); //release kernel lock
	return currentCondIndex;
}

void Wait_sys(int lockIndex, int conditionIndex) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	if (conditionIndex < 0 || conditionIndex >= currentThread->space->condCount){
		printf("waiting invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if (lockIndex < 0 || lockIndex >= currentThread->space->lockCount){
		printf("invalid lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if (currentThread->space->userConds[conditionIndex].isDeleted == TRUE){
		printf("waiting destroyed cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if (currentThread->space->userLocks[lockIndex].isDeleted == TRUE){
		printf("waiting destroyed lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// if(currentThread->space->userConds[conditionIndex].inUse == TRUE){
	// 	printf("cond already waiting\n");
	// 	currentThread->space->condsLock->Release();
	// 	return;
	// }

	// how does a lock use a wait?
	// currentThread->space->userConds[conditionIndex].inUse = TRUE;

	currentThread->space->condsLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is waited on by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	//updateProcessThreadCounts(currentThread->space, SLEEP);
	currentThread->space->userConds[conditionIndex].userCond->Wait(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

void Signal_sys(int lockIndex, int conditionIndex) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	if(conditionIndex < 0 || conditionIndex >= currentThread->space->condCount){
		printf("signaling invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if(lockIndex < 0 || lockIndex >= currentThread->space->lockCount){
		printf("invalid lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if(currentThread->space->userConds[conditionIndex].isDeleted == TRUE){
		printf("signaling destroyed cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if(currentThread->space->userLocks[lockIndex].isDeleted == TRUE){
		printf("signaling destroyed lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// if(currentThread->space->userConds[conditionIndex].inUse == TRUE){
	// 	printf("cond not waiting, can't signal\n");
	// 	currentThread->space->condsLock->Release();
	// 	return;
	// }

	// currentThread->space->userConds[conditionIndex].inUse = FALSE;

	currentThread->space->condsLock->Release(); //release kernel lock
	printf("Condition  number %d, name %s is signalled by %s\n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	// if(currentThread->space->userLocks[lockIndex].userLock->waitQueueIsEmpty()) {
	// 	//updateProcessThreadCounts(currentThread->space, AWAKE);
	// }
	currentThread->space->userConds[conditionIndex].userCond->Signal(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

void Broadcast_sys(int lockIndex, int conditionIndex) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	if(conditionIndex < 0 || conditionIndex >= currentThread->space->condCount){
		printf("broadcast invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if(lockIndex < 0 || lockIndex >= currentThread->space->lockCount){
		printf("invalid lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if(currentThread->space->userConds[conditionIndex].isDeleted == TRUE){
		printf("signaling destroyed cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if(currentThread->space->userLocks[lockIndex].isDeleted == TRUE){
		printf("signaling destroyed lock\n");
		currentThread->space->condsLock->Release();
		return;
	}

	// currentThread->space->userConds[conditionIndex].inUse = TRUE;

	currentThread->space->condsLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is broadcast by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	currentThread->space->userConds[conditionIndex].userCond->Broadcast(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

void DestroyCondition_sys(int index) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	if (index < 0 || index >= currentThread->space->condCount){
		printf("destroying invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if (currentThread->space->userConds[index].isDeleted == TRUE){
		printf("destroying invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	currentThread->space->userConds[index].deleteFlag = TRUE; // s
	if (!currentThread->space->userConds[index].userCond->waitQueueIsEmpty()){
		printf("cond still in use\n");
		currentThread->space->condsLock->Release();
		return;
	}
	if (currentThread->space->userConds[index].deleteFlag){
		printf("Condition  number %d, name %s is destroyed by %s \n", index, currentThread->space->userConds[index].userCond->getName(), currentThread->getName());
		delete currentThread->space->userConds[index].userCond;
	}
	currentThread->space->condsLock->Release();//release kernel lock
}
