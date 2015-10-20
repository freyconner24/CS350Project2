/* exectests.c
 *	Simple program to test exec syscalls
 */

#include "syscall.h"

int main(){
  Write("Testing Exec Calls\n", 19, ConsoleOutput);
  Exec("../test/matmult");
  Exec("../test/testfiles");

  Write("End of Exec Calls\n", 18, ConsoleOutput);

  Exit(0);
}
