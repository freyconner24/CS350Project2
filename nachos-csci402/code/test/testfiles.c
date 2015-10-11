/* testfiles.c
 *	Simple program to test the file handling system calls
 */

#include "syscall.h"

int main() {
  OpenFileId fd;
  int bytesread, lockNum, condNum;
  char buf[20];

 Write("Testing Locks\n", 14, ConsoleOutput);
 
 lockNum = CreateLock("nameLock");
 Acquire(lockNum);
 condNum = CreateCondition("someCondition");
 Signal(lockNum, condNum);
 Broadcast(lockNum, condNum);
Release(lockNum);
DestroyLock(lockNum);
 Write("Locks complete\n", 15, ConsoleOutput);
}
