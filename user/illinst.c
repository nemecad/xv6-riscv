#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("illinst: intentional illegal instruction execution\n");
  asm volatile(".word 0");
  exit(0);
}
