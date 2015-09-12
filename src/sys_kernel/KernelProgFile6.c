#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
int main(){
  int epfd;
  epfd=epoll_create(10);
  struct epoll_event *events;
  events = malloc (sizeof (struct epoll_event)*2);
  int ret;int nr_events;
  events[0].data.fd = 0;
  events[0].events = EPOLLIN;
  events[1].data.fd = 1;
  events[1].events = EPOLLOUT;
  ret = epoll_ctl (epfd, EPOLL_CTL_ADD, 0, &events[0]);
  if (ret)
   perror ("epoll_ctl");
  ret = epoll_ctl (epfd, EPOLL_CTL_ADD, 1, &events[1]);
  if (ret)
   perror ("epoll_ctl");

  nr_events = epoll_wait (epfd, events, 2, 5000);
  int i;
  for (i = 0; i < nr_events; i++) {
   printf ("event=%ld on fd=%d\n",
   events[i].events,
   events[i].data.fd);
   struct epoll_event *temp;
   temp = malloc(sizeof(struct epoll_event);
   temp.data.fd=events[i].data.fd;
   temp.events=events[i].events;
     ret = epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, temp);
     free(temp);
  }


  nr_events = epoll_wait (epfd, events, 2, 5000);
  for (i = 0; i < nr_events; i++) {
   printf ("event=%ld on fd=%d\n",
   events[i].events,
   events[i].data.fd);
     ret = epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[0]);
  }
}
