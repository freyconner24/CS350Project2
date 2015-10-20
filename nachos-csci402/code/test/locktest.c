/* lock_test.c
 *	Simple program to test the file handling system calls
 */

#include "syscall.h"

int lock1;
int theLockThatDoesntExist;

void t1(){
	Write("t1 acquiring theLockThatDoesntExist\n", 36, ConsoleOutput);
	Acquire(theLockThatDoesntExist);
	Write("t1 acquiring lock1\n", 19, ConsoleOutput);
	Acquire(lock1);
	Write("t1 releasing lock1\n", 19, ConsoleOutput);
	Release(lock1);
	DestroyLock(lock1);
	Exit(0);
	
}

void t2(){
	Write("t2 acquiring theLockThatDoesntExist\n", 36, ConsoleOutput);
	Acquire(theLockThatDoesntExist);
	Write("t2 acquiring lock1\n", 19, ConsoleOutput);
	Acquire(lock1);
	Exit(0);

}

int main() {
  lock1 = CreateLock("Lock1");
  theLockThatDoesntExist = lock1+10;
	Fork(t1);
  Fork(t2);
	/*Write("Testing Locks\n", 14, ConsoleOutput)
	lockNum = CreateLock("nameLock");
	Acquire(lockNum);
	condNum = CreateCondition("someCondition");
	Signal(lockNum, condNum);
	Broadcast(lockNum, condNum);
	Release(lockNum);
	DestroyLock(lockNum);
	Write("Locks complete\n", 15, ConsoleOutput);
	Exec("../test/halt");
	Exec("../test/halt");
	*/
	Write("Finshing lockfiles.c\n", 22, ConsoleOutput);
	Exit(0);
}
