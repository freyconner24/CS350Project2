#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE 32

int CreateCondition_sys(int vaddr) {
	kernelLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode

	char* buffer = new char[BUFFER_SIZE + 1];
	buffer[BUFFER_SIZE] = '\0'; //end the char array with a null character

	copyin(vaddr, BUFFER_SIZE, buffer); //copy contents of the virtual addr (ReadRegister(4)) to the buffer
	if (currentThread->space->condCount < 0 || currentThread->space->condCount >= MAX_COND_COUNT){
		DEBUG('b',"ERROR IN CONDCOUNT\n");
	}

	currentThread->space->userConds[currentThread->space->condCount].userCond = new Condition(buffer); // instantiate new cond
	currentThread->space->userConds[currentThread->space->condCount].deleteFlag = FALSE; // indicate the lock is not to be deleted

	int currentCondIndex = currentThread->space->condCount; // save the currentlockcount to be returned later
	currentThread->space->condCount++;
	printf("Condition  number %d  and name %s is created by %s \n", currentCondIndex, currentThread->space->userConds[currentCondIndex].userCond->getName(), currentThread->getName());

	kernelLock->Release(); //release kernel lock
	return currentCondIndex;
}

void Wait_sys(int lockIndex, int conditionIndex) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	currentThread->space->userConds[conditionIndex].inUse = TRUE;

	kernelLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is waited on by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	updateProcessThreadCounts(currentThread->space, SLEEP);
	currentThread->space->userConds[conditionIndex].userCond->Wait(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

void Signal_sys(int lockIndex, int conditionIndex) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	currentThread->space->userConds[conditionIndex].inUse = TRUE;

	kernelLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is signalled by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	if(!currentThread->space->userLocks[lockIndex].userLock->waitQueueIsEmpty()) {
		updateProcessThreadCounts(currentThread->space, AWAKE);
	}
	currentThread->space->userConds[conditionIndex].userCond->Signal(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

void Broadcast_sys(int lockIndex, int conditionIndex) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	currentThread->space->userConds[conditionIndex].inUse = TRUE;

	kernelLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is broadcast by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	currentThread->space->userConds[conditionIndex].userCond->Broadcast(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

void DestroyCondition_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	currentThread->space->userConds[index].deleteFlag = TRUE; // set delete flag to true regardless
	if (currentThread->space->userConds[index].deleteFlag && !currentThread->space->userConds[index].inUse){
		printf("Condition  number %d, name %s is destroyed by %s \n", index, currentThread->space->userConds[index].userCond->getName(), currentThread->getName());
		delete currentThread->space->userConds[index].userCond;
	}
	kernelLock->Release();//release kernel lock
}
