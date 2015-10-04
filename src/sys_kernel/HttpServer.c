#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "http_parser.h"
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "http_parser.h"
#include <error.h>
#include <fcntl.h>
int create_socket;
int epfd;
void network_thread_function(){
  int nr_events;
  struct epoll_event *events;
  // int bufsize = 1024*1024;
  // char *buffer = malloc(bufsize);
  events = malloc (sizeof (struct epoll_event)*2);
  int ret,i,received;
  char buf[24*4096];
  char body[1024*1024];
char resp[9999];
int pret, minor_version;
struct phr_header headers[100];
size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
ssize_t rret; char *method, *path;


  while(1){
//    printf("%s\n","waiting ..." );
    nr_events = epoll_wait (epfd, events, 100, 50);
    for (i = 0; i < nr_events; i++) {
//      printf(" nr events %d\n",nr_events );
    //  received=recv(events[i].data.fd, buffer, bufsize, 0);
///////////////////////////////////////////////////////////////////////////////////////////
    int bindex=0; int headerover=0;

    /* read the request */
    rret=recv(events[i].data.fd, buf ,24*4096,0);
    int j=0;
    int end=rret-4;
        while(j<=end){
          if(headerover==0){
            if(buf[j]=='\r' && buf[j+1]=='\n' && buf[j+2]=='\r' && buf[j+3]=='\n'){
              j=j+3;end=rret+4; headerover=1;
            }
          }
          if (headerover==1) {
            body[bindex++]=buf[j];
          }
          j++;
        }

    prevbuflen = buflen;
    buflen += rret;
    /* parse the request */
    num_headers = sizeof(headers) / sizeof(headers[0]);

    pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                             &minor_version, headers, &num_headers, 0);

//    printf("%d",minor_version );
if(headerover==1){
  for(j=0;j<=bindex;j++){
  //  printf("%c",body[j]);
  }
}

int keepalive=0;
if(minor_version==1)
    {keepalive=1;}
    int k=0;
for (k = 0; k != num_headers; ++k) {
//    printf("%.*s: %.*s\n", (int)headers[k].name_len, headers[k].name,(int)headers[k].value_len, headers[k].value);
  if (strcmp(headers[k].name,"Connection")==0) {
    if (headers[k].value[0]=='K'||headers[k].value[1]=='K') {
      keepalive=1;
    }
  }
         }
////////////////////////////////////////////////////////////////////////////////////////////

// int keepalive=1;
if (keepalive == 1) {

strcpy(resp,"HTTP/1.1 200 OK\nContent-length: 47\nContent-Type: text/html\n\n<html><body><H1>Hello Kartik</H1></body></html>");

int c=send(events[i].data.fd,&resp,sizeof(resp),MSG_DONTWAIT|MSG_NOSIGNAL);
  //printf("%s\n","going to send" );
//  send(events[i].data.fd, "HTTP/1.1 200 OK\nContent-length: 46\nContent-Type: text/html\n\n<html><body><H1>Hello world</H1></body></html>", 110,0);
//  printf("%s\n","sent with keepalive connection" );
       //ret = epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
    //   close(events[i].data.fd);

}else{

strcpy(resp,"HTTP/1.1 200 OK\nContent-length: 47\nContent-Type: text/html\n\n<html><body><H1>Hello Kartik</H1></body></html>");

int c=send(events[i].data.fd,&resp,sizeof(resp),MSG_DONTWAIT|MSG_NOSIGNAL);
//  printf("%s\n","going to send for non keepalive" );
//  send(events[i].data.fd, "HTTP/1.1 200 OK\nContent-length: 46\nContent-Type: text/html\n\n<html><body><H1>Hello world</H1></body></html>", 110,0);
//printf("%s\n","sent for non keepalive" );
       ret = epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
      close(events[i].data.fd);

}



 }
}
}
  void sigint_handler(int sig); /* prototype */
void sigint_handler(int sig)
{
    close(create_socket);
    exit(1);
}

int main() {

    char s[200];
    struct sigaction sa;

    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0; // or SA_RESTART
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT | SIGTERM | SIGPIPE | SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
   int  new_socket;
   socklen_t addrlen,clientaddresslen;
   int bufsize = 1024;
   char *buffer = malloc(bufsize);
   struct sockaddr_in address,clientaddress;
   memset((void*)&address,(int)'\0',sizeof(address));
   memset((void*)&clientaddress,(int)'\0',sizeof(clientaddress));

   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
      printf("The socket was created\n");
   }

   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(5000);

   if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
      printf("Binding Socket\n");
   }
   int optval = 1;
   setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
   epfd=epoll_create(10000);
   struct epoll_event *events;
   events = malloc (sizeof (struct epoll_event)*2);
   int ret,i;
   pthread_t threads[4];
   for (i = 0; i < 4; i++) {
     pthread_create( &threads[i], NULL, &network_thread_function, NULL);
   }

   while (1) {
      if (listen(create_socket, 10000) < 0) {
         perror("server: listen");
         //exit(1);
      }

      if ((new_socket = accept4(create_socket, (struct sockaddr *) &clientaddress, &clientaddresslen,SOCK_NONBLOCK)) < 0) {
         perror("server: accept");
         //exit(1);
      }

      if (new_socket > 0){
        events[0].data.fd = new_socket;
        events[0].events = EPOLLIN | EPOLLET;

        ret = epoll_ctl (epfd, EPOLL_CTL_ADD, events[0].data.fd, &events[0]);
      }


   }
   close(create_socket);
   return 0;
}
