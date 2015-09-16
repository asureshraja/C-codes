#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>


int main()
{
  int i;
  for(i = 0; i < 4; i++) {
    pid_t pid = fork();
    if(pid < 0) {
        printf("Error");
        exit(1);
    } else if (pid == 0) {
        printf("Child (%d): %d\n", i + 1, getpid());
        exit(0);
    } else  {
        wait(NULL);
    }
}



}
