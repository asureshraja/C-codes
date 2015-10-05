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
int server_socket;
int epfd;   int  new_socket;
void network_thread_function(){
  int nr_events;
  struct epoll_event *events;
  events = malloc (sizeof (struct epoll_event)*2);
  int ret,i,received;
  char buf[24*4096];
  char body[1024*1024];
char str_key[100],str_value[100];
int pret, minor_version;
struct phr_header headers[100];
size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
ssize_t rret; char *method, *path;
    int bindex=0; int headerover=0;
    int j=0;  int end=rret-4;
  while(1){

    nr_events = epoll_wait (epfd, events, 1, -1);
    for (i = 0; i < nr_events; i++) {

     bindex=0;  headerover=0;

    rret=recv(events[i].data.fd, buf ,24*4096,0);
     j=0;
     end=rret-4;
        while(j<end){
          if(headerover==0){
            if(buf[j]=='\r' && buf[j+1]=='\n' && buf[j+2]=='\r' && buf[j+3]=='\n'){
              j=j+4;end=rret+4; headerover=1;
            }
          }
          if (headerover==1) {
            body[bindex++]=buf[j];
          }
          j++;
        }


    buflen += rret;

    num_headers = sizeof(headers) / sizeof(headers[0]);

    pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                             &minor_version, headers, &num_headers, 0);

  //  printf("minor version %d",minor_version );
if(headerover==1){
  for(j=0;j<=bindex;j++){
    //printf("%c",body[j]);
    //printf("%d\n",(int)sizeof(body));
  }
}

int keepalive=0;
if(minor_version==1)
    {keepalive=1;}
    int k=0;
for (k = 0; k != num_headers; ++k) {
//    printf("%.*s: %.*s\n", (int)headers[k].name_len, headers[k].name,(int)headers[k].value_len, headers[k].value);
sprintf(str_key,"%.*s", (int)headers[k].name_len, headers[k].name);
sprintf(str_value,"%.*s", (int)headers[k].value_len, headers[k].value);
  if (strcmp(str_key,"Connection")==0) {
    if (str_value[0]=='K'||str_value[1]=='K'||str_value[0]=='k'||str_value[1]=='k') {
      keepalive=1;
    }
  }
         }

if (keepalive == 1) {
  if(minor_version==1)
      {
 send(events[i].data.fd,"HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",106,MSG_DONTWAIT|MSG_NOSIGNAL);
}else{
  send(events[i].data.fd,"HTTP/1.0 200 OK\nConnection: keep-alive\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",129,MSG_DONTWAIT|MSG_NOSIGNAL);
}

}else{
  if(minor_version==1)
      {
          send(events[i].data.fd,"HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",124,MSG_DONTWAIT|MSG_NOSIGNAL);
      }else{
          send(events[i].data.fd,"HTTP/1.0 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",124,MSG_DONTWAIT|MSG_NOSIGNAL);

      }


       ret = epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
      close(events[i].data.fd);


}



 }
}
}


int main() {

   socklen_t addrlen,clientaddresslen;
   int bufsize = 1024;
   char *buffer = malloc(bufsize);
   struct sockaddr_in address,clientaddress;
   memset((void*)&address,(int)'\0',sizeof(address));
   memset((void*)&clientaddress,(int)'\0',sizeof(clientaddress));

   if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
      printf("The socket was created\n");
   }

   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(5000);

   if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
      printf("Binding Socket\n");
   }
   int optval = 1;
   setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR|SO_LINGER, &optval, sizeof(optval));

   epfd=epoll_create(10000);
   struct epoll_event *events;
   events = malloc (sizeof (struct epoll_event)*2);
   int ret,i;
   pthread_t threads[4];
   for (i = 0; i < 4; i++) {
     pthread_create( &threads[i], NULL, &network_thread_function, NULL);
   }
   while (1) {
      if (listen(server_socket, 10000) < 0) {
         perror("server: listen");
         //exit(1);
      }

      if ((new_socket = accept4(server_socket, (struct sockaddr *) &clientaddress, &clientaddresslen,SOCK_NONBLOCK)) < 0) {
         perror("server: accept");
         //exit(1);
      }

      if (new_socket > 0){
        events[0].data.fd = new_socket;
        events[0].events = EPOLLIN | EPOLLET;
        ret = epoll_ctl (epfd, EPOLL_CTL_ADD, events[0].data.fd, &events[0]);
      }

   }
   close(server_socket);
   return 0;
}
