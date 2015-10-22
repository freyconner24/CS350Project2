#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include "addrspace.h"
#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE 32

// the create condition syscall
// this essentailly makes a call to new Condition() inside synch.h
int CreateCondition_sys(int vaddr, int size, int appendNum) {
	currentThread->space->condsLock->Acquire(); //CL: acquire kernelLock so that no other thread is running on kernel mode
	// checks if the number is greater than then max count
	if (currentThread->space->condCount == MAX_COND_COUNT){
		DEBUG('r',"    CreateCondition::");
		DEBUG('r',currentThread->getName());
		DEBUG('r'," has too many conditions!\n");
		currentThread->space->condsLock->Release();
		return -1;
	}

	// checks for an invalid string size
	if(size < 0 || size > 32) {
		DEBUG('r',"    CreateCondition::");
		DEBUG('r',currentThread->getName());
		DEBUG('r'," size must be: (size < 0 || size > 32) ? %d\n", size);
		currentThread->space->condsLock->Release();
		return -1;
	}

	char* buffer = new char[size + 1];
	buffer[size] = '\0'; //end the char array with a null character

	if(copyin(vaddr, size, buffer) == -1) { //copy contents of the virtual addr (ReadRegister(4)) to the buffer
		DEBUG('r',"    CreateCondition::copyin failed\n");
		delete [] buffer;
		currentThread->space->condsLock->Release();
		return -1;
	}
	if (currentThread->space->condCount < 0 || currentThread->space->condCount >= MAX_COND_COUNT){
		DEBUG('r',"ERROR IN CONDCOUNT\n");
		DEBUG('r',"    CreateCondition::condCount is neg or greater than max count: %d\n", currentThread->space->condCount);
		currentThread->space->condsLock->Release();
		return -1;
	}

	currentThread->space->userConds[currentThread->space->condCount].userCond = new Condition(buffer); // instantiate new lock
	currentThread->space->userConds[currentThread->space->condCount].deleteFlag = FALSE; // indicate the lock is not to be deleted
	currentThread->space->userConds[currentThread->space->condCount].isDeleted = FALSE; // indicate the lock is not in use
	int currentCondIndex = currentThread->space->condCount; // save the currentlockcount to be returned later
	currentThread->space->condCount++;
	DEBUG('r',"    CreateCondition::Condition number: %d || name: %s is created by %s\n", currentCondIndex, currentThread->space->userConds[currentCondIndex].userCond->getName(), currentThread->getName());

	currentThread->space->condsLock->Release(); //release kernel lock
	return currentCondIndex;
}

// the Wait syscall
// this essentailly appends a lock to the waitQueue inside the condition var
void Wait_sys(int lockIndex, int conditionIndex) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	// checks bad condition index
	if (conditionIndex < 0 || conditionIndex >= currentThread->space->condCount){
		DEBUG('r',"    Wait::invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// checks bad lock index
	if (lockIndex < 0 || lockIndex >= currentThread->space->lockCount){
		DEBUG('r',"    Wait::invalid lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// check if the condition exists
	if (currentThread->space->userConds[conditionIndex].isDeleted == TRUE){
		DEBUG('r',"    Wait::destroyed cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// check if the lock exists
	if (currentThread->space->userLocks[lockIndex].isDeleted == TRUE){
		DEBUG('r',"    Wait::destroyed lock\n");
		currentThread->space->condsLock->Release();
		return;
	}

	// how does a lock use a wait?
	currentThread->space->condsLock->Release();//release kernel lock
	DEBUG('r',"    Wait::Condition  number %d, name %s is waited on by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	//updateProcessThreadCounts(currentThread->space, SLEEP);
	currentThread->space->userConds[conditionIndex].userCond->Wait(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index
}

// the Signal syscall
// this essentailly signals a lock off of the waitQueue 
void Signal_sys(int lockIndex, int conditionIndex) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	// checks bad condition index
	if(conditionIndex < 0 || conditionIndex >= currentThread->space->condCount){
		DEBUG('r',"    Signal::invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// checks bad lock index
	if(lockIndex < 0 || lockIndex >= currentThread->space->lockCount){
		DEBUG('r',"    Signal::invalid lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// check if the condition exists
	if(currentThread->space->userConds[conditionIndex].isDeleted == TRUE){
		DEBUG('r',"    Signal::destroyed cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// check if the lock exists
	if(currentThread->space->userLocks[lockIndex].isDeleted == TRUE){
		DEBUG('r',"    Signal::destroyed lock\n");
		currentThread->space->condsLock->Release();
		return;
	}

	currentThread->space->condsLock->Release(); //release kernel lock
	DEBUG('r',"    Condition  number %d, name %s is signalled by %s\n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	// the actual signal call
	currentThread->space->userConds[conditionIndex].userCond->Signal(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index

	// checks if the condition var has empty waitQueue and flag so that it can be deleted
	if (currentThread->space->userConds[conditionIndex].deleteFlag && 
		currentThread->space->userConds[conditionIndex].userCond->waitQueueIsEmpty()){
		DEBUG('r',"    DestroyCondition::Condition  number %d, name %s is destroyed by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());
		delete currentThread->space->userConds[conditionIndex].userCond;
		currentThread->space->userConds[conditionIndex].isDeleted = TRUE;
	}
}

// the Broadcast syscall
// signals all of the threads inside the address space.
void Broadcast_sys(int lockIndex, int conditionIndex) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	// checks bad condition index
	if(conditionIndex < 0 || conditionIndex >= currentThread->space->condCount){
		DEBUG('r',"    Broadcast::invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// checks bad lock index
	if(lockIndex < 0 || lockIndex >= currentThread->space->lockCount){
		DEBUG('r',"    Broadcast::invalid lock\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// check if the condition exists
	if(currentThread->space->userConds[conditionIndex].isDeleted == TRUE){
		DEBUG('r',"    Broadcast::signaling destroyed cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// check if the lock exists
	if(currentThread->space->userLocks[lockIndex].isDeleted == TRUE){
		DEBUG('r',"    Broadcast::signaling destroyed lock\n");
		currentThread->space->condsLock->Release();
		return;
	}

	currentThread->space->condsLock->Release();//release kernel lock
	DEBUG('r',"    Broadcast::Condition  number %d, name %s is broadcast by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());

	// calls the actual Broadcast
	currentThread->space->userConds[conditionIndex].userCond->Broadcast(currentThread->space->userLocks[lockIndex].userLock); // acquire userlock at index

	if (currentThread->space->userConds[conditionIndex].deleteFlag){
		DEBUG('r',"    DestroyCondition::Condition  number %d, name %s is destroyed by %s \n", conditionIndex, currentThread->space->userConds[conditionIndex].userCond->getName(), currentThread->getName());
		delete currentThread->space->userConds[conditionIndex].userCond;
		currentThread->space->userConds[conditionIndex].isDeleted = TRUE;
	}
}

// the destroy condition syscall
// flags the condition to be deleted
void DestroyCondition_sys(int index) {
	currentThread->space->condsLock->Acquire(); // CL: acquire kernelLock so that no other thread is running on kernel mode
	// checks bad condition index
	if (index < 0 || index >= currentThread->space->condCount){
		DEBUG('r',"    DestroyCondition::destroying invalid cond\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// checks bad lock index
	if (currentThread->space->userConds[index].isDeleted == TRUE){
		DEBUG('r',"    DestroyCondition::condition is already destroyed\n");
		currentThread->space->condsLock->Release();
		return;
	}
	// checks if the condition waitQueue is not empty
	if (!currentThread->space->userConds[index].userCond->waitQueueIsEmpty()){
		DEBUG('r',"    DestroyCondition::cond still in use\n");
		// flags the delete
		currentThread->space->userConds[index].deleteFlag = TRUE; // s
		currentThread->space->condsLock->Release();
		return;
	} else {
		//if the waitQueue is empty, the condition can be deleted
		currentThread->space->userConds[index].deleteFlag = TRUE; // s
		delete currentThread->space->userConds[conditionIndex].userCond;
		currentThread->space->userConds[conditionIndex].isDeleted = TRUE;
	}
	
	currentThread->space->condsLock->Release();//release kernel lock
}
