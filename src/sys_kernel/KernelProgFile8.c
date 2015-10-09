#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <stdlib.h>

int epollfd, _eventfd;

int init()
{
    epollfd = epoll_create1(0);
    _eventfd = eventfd(0, EFD_NONBLOCK);
    struct epoll_event evnt = {0};
    evnt.data.fd = _eventfd;
    evnt.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, _eventfd, &evnt);
}

void *work(void *arg)
{
    static const int EVENTS = 20;
    struct epoll_event *events=malloc(sizeof(struct epoll_event)*EVENTS);
    while (1) {
        int count = epoll_wait(epollfd, events, EVENTS, -1);
        int i;
        for (i = 0; i < count; ++i)
        {
            if (events[i].data.fd==_eventfd){
                eventfd_t val;
                eventfd_read(_eventfd, &val);
                printf(" (pthread id %d) has started\n", pthread_self());
                printf("DING: %lld\n", (long long)val);
            }
        }
    }
}

int main()
{
    pthread_t workers[4];
    init();int i;
    for (i = 0; i < 4; i++) {
      pthread_create( &workers[i], NULL, work, NULL);
    }
    printf("eventfd_write\n");
    eventfd_write(_eventfd, 1);eventfd_write(_eventfd, 1);
    eventfd_write(_eventfd, 1);
}
