#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "thpool.h"
int make_socket_non_blocking (int );
int server_socket, new_socket;threadpool thpool;
socklen_t addrlen;
struct sockaddr_in address;
int epfd;
struct epoll_event *events;
void *write_data(void *i){
  int socket_fd = *((int *) i);
  //make_socket_non_blocking(socket_fd);
  write(socket_fd, "hello world\n", 12);
  close(socket_fd);

}
void *iothread_execute(){
  //child process works here
  printf("inside thread\n" );

  int ret1;int nr_events1;
struct epoll_event *events1 = malloc(sizeof(struct epoll_event)*10000);
char *buffer = malloc(sizeof(char)*1024);
  int i;
  while (1) {
    //printf("over epfd" );
    nr_events1 = epoll_wait (epfd, events1, 1, -1);
    //printf("wai over epfd" );
    for ( i = 0; i < nr_events1; i++) {

           recv(events1[i].data.fd, buffer, 1024*1024, 0);
           //printf("%s\n", buffer);

          //  write(events1[i].data.fd, "hello world\n", 12);
          //  close(events1[i].data.fd);
          int *arg =&events1[i].data.fd;

          thpool_add_work(thpool, (void*)write_data,arg);
            ret1 = epoll_ctl (epfd, EPOLL_CTL_DEL, events1[i].data.fd, &events1[i]);

    }


  }
}
int main() {
  thpool = thpool_init(12);
  int bufsize = 1024;
  char *buffer = malloc(bufsize);
  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
     printf("The socket was created\n");
  }


  //address = malloc(sizeof(struct sockaddr_in));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8025);
  //make_socket_non_blocking(server_socket);
  if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
     printf("Binding Socket\n");
  }


    epfd=epoll_create(1000);

    events = malloc (sizeof (struct epoll_event)*10000);
    int ret;int nr_events;


    int num_threads=4;
  pthread_t threads[num_threads];
  int t=0;
      for(t=0; t<num_threads; t++){

         pthread_create(&threads[t], NULL, iothread_execute, NULL);
      }
//make_socket_non_blocking(server_socket);
  while (1) {

     if (listen(server_socket, 10000) < 0) {
        //perror("server: listen");
     }

     if ((new_socket = accept(server_socket, (struct sockaddr *) &address, &addrlen)) < 0) {
    //    perror("server: accept");

     }
    // make_socket_non_blocking(new_socket);
    // printf("data\n");
    //make_socket_non_blocking(new_socket);
     events[0].data.fd = new_socket;
     events[0].events = EPOLLIN | EPOLLET;
     ret = epoll_ctl (epfd, EPOLL_CTL_ADD, new_socket, &events[0]);
  }

  for(t=0; t<num_threads; t++){
    pthread_join(threads[t], NULL);
  }


close(server_socket);
thpool_destroy(thpool);
  return 0;
}







int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}
