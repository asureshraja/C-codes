#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
int main(void) {
  struct pollfd fds[2];
  fds[0].fd=0;fds[1].fd=1;
  fds[0].events=POLLIN;fds[1].events=POLLOUT;//fd 1 is always writable.(STDOUT)

  int ret=poll(fds,2, 5000);
  if(ret==-1){
    perror("poll");
  }
  if (fds[0].revents==POLLIN) {
    printf("ready to read\n");
  }
  if (fds[1].revents==POLLOUT) {
    printf("ready to write\n");
  }
}
