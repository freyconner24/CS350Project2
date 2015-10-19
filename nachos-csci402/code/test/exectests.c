/* exectests.c
 *	Simple program to test exec syscalls
 */

#include "syscall.h"

int main(){
  Exec("../test/testfiles");
  Exec("../test/testfiles");
}
