/* testfiles.c
 *	Simple program to test the file handling system calls
 */

#include "syscall.h"

#define NUM_FORK_CALLS 10
/* ++++++++++++++++++++++++++++++++++++++++ */

int totalForkCalls = 0;

/* ---------------------------------------- */

void helloWorld(){
	Write("Hello World!\n", 13, ConsoleOutput);
	Exit(0);
}

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++ */

void printForkName(char* str) {
    /*char* forkString = concatStringWithNumber("ForkCall_", str);*/
    /*writeWithSize(forkString);*/
    ++totalForkCalls;
}

/*
void testForkCall() {
    int i;
    for(i = 0; i < NUM_FORK_CALLS; ++i) {
        Fork(printForkName, i);
    }
}
*/
/* ---------------------------------------------------- */

int main() {
	OpenFileId fd;
	int bytesread, lockNum, condNum, i;
	char buf[20];

	/*
	clerkLineLock = CreateLock("ClerkLineLock");
	clerkSenatorLineCV = CreateCondition("ClerkSenatorLineCV");
	outsideLineCV = CreateCondition("OutsideLineCV");
	outsideLock = CreateLock("OutsideLock");
	senatorLock = CreateLock("SenatorLock");
	senatorLineCV = CreateCondition("SenatorLineCV");
	*/

	/*Write("Testing Locks\n", 14, ConsoleOutput);

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
  testForkCall();
	*/

/*	Exec('halt');*/

	Write("Forking helloWorld\n", 19, ConsoleOutput);
	Fork(helloWorld, 0);
<<<<<<< Updated upstream
=======

>>>>>>> Stashed changes
		Fork(helloWorld, 0);
			Fork(helloWorld, 0);
				Fork(helloWorld, 0);
					Fork(helloWorld, 0);
						Fork(helloWorld, 0);
							Fork(helloWorld, 0);
								Fork(helloWorld, 0);
									Fork(helloWorld, 0);
										Fork(helloWorld, 0);

										Fork(helloWorld, 0);

	Write("Finshing testfiles.c\n", 21, ConsoleOutput);
	Exit(0);
}
