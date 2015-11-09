#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "http_parser.h"
#include <netinet/tcp.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "http_parser.h"
#include <error.h>
#include "server.h"
#include <fcntl.h>
#include "thpool.h"
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sched.h>
#include <search.h>
#include <signal.h>

struct req_state{
  int direction; //0 for forward and 1 for reverse
  int client_sock_id;
  int server_sock_id;
};
int req_counter=0;static rec_count=0;
int server_socket;char *forward;
char *reverse;
int epfd;int num_of_connections=100;
int NUMBER_OF_REMOTE_IPS=2;
char ips[15][50];
int ports[15]; int sock[200];
int stick_this_thread_to_core() {
      static int core_id=-1;
        core_id=core_id+1;
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   core_id=core_id%num_cores;
   if (core_id < 0 || core_id >= num_cores)
      return -1;
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void proxy_function(){
  stick_this_thread_to_core();
    int number_of_ready_events;
    struct epoll_event *events;
    int max_events_to_stop_waiting=100;
    int epoll_time_wait=500;char read_buffer[1024*256];
    char out_buffer[1024*256];
    int got=0,i=0;  struct epoll_event *events1;int ret;
    events1 = malloc (sizeof (struct epoll_event)*2); //epoll event allocation for multiplexing
    events = malloc (sizeof (struct epoll_event)*max_events_to_stop_waiting); //epoll event allocation for multiplexing
    while(1){
      number_of_ready_events = epoll_wait (epfd, events, max_events_to_stop_waiting, epoll_time_wait);
      for (i = 0; i < number_of_ready_events; i++) {

              read_buffer[0]='\0';
              // printf("%d\n",events[i].data.fd );
              got=recv(events[i].data.fd, read_buffer ,1024*256,MSG_NOSIGNAL);
              if(got==0 || got ==-1){
                continue;
              }
              // printf("from client bytes is      %d\n",got );


              read_buffer[got]='\0';
              int conn_to_forward=sock[(req_counter++)%(num_of_connections*NUMBER_OF_REMOTE_IPS)];
              send(conn_to_forward,read_buffer,strlen(read_buffer),MSG_NOSIGNAL);


            //  printf("%s\n","response sending to cliet" );
              out_buffer[0]='\0';

              got=recv(conn_to_forward,out_buffer ,1024*256,MSG_NOSIGNAL);
              out_buffer[got]='\0';
            //  printf("got %d\n", got);
            //  printf(" msg %s\n",out_buffer );
              send(events[i].data.fd,out_buffer,strlen(out_buffer),MSG_DONTWAIT|MSG_NOSIGNAL);

      }
    }

}

void accept_connections(){
        stick_this_thread_to_core();
        int new_socket=0;socklen_t clientaddresslen;
        struct sockaddr_in clientaddress; int ret; int opt=1;
        struct epoll_event *events1;
        events1 = malloc (sizeof (struct epoll_event)*2); //epoll event allocation for multiplexing
        memset((void*)&clientaddress,(int)'\0',sizeof(clientaddress));
        clientaddresslen=sizeof(clientaddress);
        while(1){
      //    printf("%s\n","waiting to accept connections" );
          if ((new_socket = accept4(server_socket, (struct sockaddr *) &clientaddress, &clientaddresslen,0)) < 0) {
             perror("server: accept");
             //exit(1);
          }

    //      printf("%s\n","new socket created" );
          if (new_socket > 0){
            setsockopt( new_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&opt, sizeof(opt));
            setsockopt( new_socket, IPPROTO_TCP, TCP_QUICKACK, (void *)&opt, sizeof(opt));
            events1[0].data.fd = new_socket;
            events1[0].events = EPOLLIN | EPOLLET;


            ret = epoll_ctl (epfd, EPOLL_CTL_ADD, new_socket, &events1[0]);
      //      printf("%s\n","request received" );
          }

        }
}

void open_gate(){
  num_of_connections=100;NUMBER_OF_REMOTE_IPS=2;
  int optval=1;int flags=0;
  socklen_t optlen = sizeof(optval);
  strcpy(ips[0],"127.0.0.1");
  strcpy(ips[1],"127.0.0.1");
  ports[0]=5000;ports[1]=5001;
  struct sockaddr_in *remote; //remote address structure
  struct epoll_event *events1;
  events1 = malloc (sizeof (struct epoll_event)*2); //epoll event allocation for multiplexing
  remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in)*NUMBER_OF_REMOTE_IPS); //allocating remote address
  int i=0;int ret;
  for (i = 0; i < NUMBER_OF_REMOTE_IPS*num_of_connections; ++i)
  {
    remote[i%NUMBER_OF_REMOTE_IPS].sin_family = AF_INET;
    inet_pton(AF_INET, ips[i%NUMBER_OF_REMOTE_IPS], (void *)(&(remote[i%NUMBER_OF_REMOTE_IPS].sin_addr.s_addr))); //setting ip
    remote[i%NUMBER_OF_REMOTE_IPS].sin_port = htons(ports[i%NUMBER_OF_REMOTE_IPS]); //setting port

    if((sock[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){ //creating tcp socket
      perror("Can't create TCP socket");
      exit(1);
    }


    if(setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &optval, optlen) < 0) {
       perror("setsockopt()");
       exit(EXIT_FAILURE);
    }
    if(setsockopt(sock[i], SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    if(connect(sock[i], (struct sockaddr *)&remote[i%NUMBER_OF_REMOTE_IPS], sizeof(struct sockaddr)) < 0){ //connecting to remote address and ataching to socket
     perror("connect error");

   }
  //  flags = fcntl(sock[i], F_GETFL, 0);
  //  fcntl(sock[i], F_SETFL, flags | O_NONBLOCK);


  //   events1[0].data.fd = sock[i];
  // //  printf("%s %d\n","adding sock conn",sock[i] );
  //   events1[0].events = EPOLLIN | EPOLLET;
    //ret = epoll_ctl (epfd, EPOLL_CTL_ADD, sock[i], &events1[0]);


  }
}

int main() {
  hcreate(3000);
   socklen_t addrlen;
   struct sockaddr_in address;
   memset((void*)&address,(int)'\0',sizeof(address));



   if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) > 0){
      printf("The socket was created\n");
   }
   int option=1;
   setsockopt( server_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&option, sizeof(option));
   setsockopt( server_socket, IPPROTO_TCP, TCP_QUICKACK, (void *)&option, sizeof(option));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(8000);

   if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
      printf("Binding Socket\n");
   }
   int optval = 1;
   setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR|SO_LINGER, &optval, sizeof(optval));

   epfd=epoll_create(100000);
   struct epoll_event *events;
   events = malloc (sizeof (struct epoll_event)*2);
   int ret,i;

   open_gate();
  //  struct epoll_event evnt;
  //  evnt.data.fd = server_socket;
  //  evnt.events = EPOLLIN | EPOLLET;
  //  epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &evnt);

   if (listen(server_socket, 10000) < 0) {
         perror("server: listen");
         //exit(1);
   }

   pthread_t threads[8];
   for (i = 0; i < 1; i++) {
     pthread_create( &threads[i], NULL, &accept_connections, NULL);
   }
   for (i = 1; i < 2; i++) {
     pthread_create( &threads[i], NULL, &proxy_function, NULL);
   }
   //stick_this_thread_to_core();
  //accept_connections();
  for ( i = 0; i < 2; i++)
       pthread_join(threads[i], NULL);

   close(server_socket);
   return 0;
}

