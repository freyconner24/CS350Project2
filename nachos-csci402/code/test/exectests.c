/* exectests.c
 *	Simple program to test exec syscalls
 */

#include "syscall.h"

int main(){
  Write("Testing Exec Calls\n", 19, ConsoleOutput);
  Exec("../test/testfiles");
  Exec("../test/testfiles");
  Exit(0);
}
