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
	if (condCount < 0 || condCount >= MAX_COND_COUNT){
		DEBUG('b',"ERROR IN CONDCOUNT\n");
	}

	userConds[condCount].userCond = new Condition(buffer); // instantiate new cond
	userConds[condCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	userConds[condCount].addrSpace = currentThread->space; // copy address space from current thread to the lock, because the lock belongs to the current thread

	int currentCondIndex = condCount; // save the currentlockcount to be returned later
	condCount++;
	printf("Condition  number %d  and name %s is created by %s \n", currentCondIndex, userConds[currentCondIndex].userCond->getName(), currentThread->getName());

	kernelLock->Release(); //release kernel lock
	return currentCondIndex;
}

void Wait_sys(int lockIndex, int conditionIndex) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	userConds[conditionIndex].inUse = TRUE;

	kernelLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is waited on by %s \n", conditionIndex, userConds[conditionIndex].userCond->getName(), currentThread->getName());

	updateProcessThreadCounts(currentThread->space, SLEEP);
	userConds[conditionIndex].userCond->Wait(userLocks[lockIndex].userLock); // acquire userlock at index
}

void Signal_sys(int lockIndex, int conditionIndex) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	userConds[conditionIndex].inUse = TRUE;

	kernelLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is signalled by %s \n", conditionIndex, userConds[conditionIndex].userCond->getName(), currentThread->getName());

	if(!userLocks[lockIndex].userLock->waitQueueIsEmpty()) {
		updateProcessThreadCounts(currentThread->space, AWAKE);
	}
	userConds[conditionIndex].userCond->Signal(userLocks[lockIndex].userLock); // acquire userlock at index
}

void Broadcast_sys(int lockIndex, int conditionIndex) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode

	userConds[conditionIndex].inUse = TRUE;

	kernelLock->Release();//release kernel lock
	printf("Condition  number %d, name %s is broadcast by %s \n", conditionIndex, userConds[conditionIndex].userCond->getName(), currentThread->getName());

	userConds[conditionIndex].userCond->Broadcast(userLocks[lockIndex].userLock); // acquire userlock at index
}

void DestroyCondition_sys(int index) {
	kernelLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	userConds[index].deleteFlag = TRUE; // set delete flag to true regardless
	if (userConds[index].deleteFlag && !userConds[index].inUse){
		printf("Condition  number %d, name %s is destroyed by %s \n", index, userConds[index].userCond->getName(), currentThread->getName());
		delete userConds[index].userCond;
	}
	kernelLock->Release();//release kernel lock
}
