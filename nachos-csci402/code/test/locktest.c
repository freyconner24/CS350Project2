/* lock_test.c
 *	Simple program to test the file handling system calls
 */

#include "syscall.h"

int lock1;
int lock2;
int theLockThatDoesntExist;
int check = 0;
int i = 0;
int deadLock1;
int deadLock2;

void t1(){
	Write("t1 releasing lock1 before acquiring\n", 35, ConsoleOutput);
	Release(lock1);
	Write("t1 acquiring theLockThatDoesntExist\n", 36, ConsoleOutput);
	Acquire(theLockThatDoesntExist);
	Write("t1 acquiring lock1\n", 19, ConsoleOutput);
	Acquire(lock1);
	for (i = 0; i < 50; ++i)
		++check;
	Write("t1 releasing theLockThatDoesntExist\n", 36, ConsoleOutput);
	Release(theLockThatDoesntExist);
	Write("t1 releasing lock1\n", 19, ConsoleOutput);
	PrintNum(check);PrintNl();
	Write("t1 destroying lock1\n", 19, ConsoleOutput);
	Release(lock1);
	Write("t1 acquiring lock2 and not releasing\n", 19, ConsoleOutput);
	Acquire(lock2);
	Exit(0);
}

void t2(){
	Write("t2 acquiring theLockThatDoesntExist\n", 36, ConsoleOutput);
	Acquire(theLockThatDoesntExist);
	Write("t2 acquiring lock1\n", 19, ConsoleOutput);
	Acquire(lock1);
	for ( i = 0; i < 1; ++i)
		check*=2;
	Write("t2 releasing lock1\n", 19, ConsoleOutput);
	PrintNum(check); PrintNl();
	Acquire(lock2);
	Exit(0);
}

int main() {
  lock1 = CreateLock("Lock1", 5, 0);
	lock2 = CreateLock("Lock2", 5, 0);
	deadLock1 = CreateLock("deadLock1", 9, 0);
	deadLock2 = CreateLock("deadLock2", 9, 0);
  theLockThatDoesntExist = lock1+10;
	Write("Testing invalid actions for locks\n", 34, ConsoleOutput);
	Write("Acquiring theLockThatDoesntExist, should give error\n", 52, ConsoleOutput);
	Acquire(theLockThatDoesntExist);
	Write("Releasing theLockThatDoesntExist, should give error\n", 52, ConsoleOutput);
	Release(theLockThatDoesntExist);
	DestroyLock(theLockThatDoesntExist);
	Fork(t1,0);
  Fork(t2,0);
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
