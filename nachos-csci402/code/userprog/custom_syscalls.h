#ifndef CUSTOM_SYSCALLS_H
#define CUSTOM_SYSCALLS_H

#include "copyright.h"


int CreateLock_sys(int vaddr);
void Acquire_sys(int lockIndex);
void Release_sys(int lockIndex);
void DestroyLock_sys(int destroyValue);

int CreateCondition_sys(int vaddr);
void Wait_sys(int lockIndex, int conditionIndex);
void Signal_sys(int lockIndex, int conditionIndex);
void Broadcast_sys(int lockIndex, int conditionIndex);
void DestroyCondition_sys(int destroyValue);

#endif /* CUSTOM_SYSCALLS_H */
