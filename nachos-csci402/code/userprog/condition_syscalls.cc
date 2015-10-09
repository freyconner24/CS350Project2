#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "custom_syscalls.h"
#include <stdio.h>
#include <iostream>

int CreateCondition_sys(int vaddr) {
	return 0;
}

void Wait_sys(int lockIndex, int conditionIndex) {

}

void Signal_sys(int lockIndex, int conditionIndex) {

}

void Broadcast_sys(int lockIndex, int conditionIndex) {

}

void DestroyCondition_sys(int destroyValue) {

}
