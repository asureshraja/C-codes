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
#include <string.h>
/*
Experimental c code reading data on http body and sending some text as response
gcc HttpServer.c http_parser.c -o server -lpthread -w
TODO
1. Refactor
2. Add lock free mpmc queue to support worker threads
3. to support controllers/handlers and routing api calls
*/
char* concatenate( char* dest, char* src );
int str_len(const char *str);
int server_socket;
int epfd;   int  new_socket;




char* concatenate( char* dest, char* src )
{
     while (*dest) dest++;
     while (*dest++ = *src++);
     return --dest;
}
int str_len(const char *str)
{
    const char *s;
    for (s = str; *s; ++s);
    return(s - str);
}
struct http_request{
    int socket_id;
    int keepalive;
    int minor_version;
    char body[512*1024];
    struct phr_header headers[100];
};
void concatenate_string(char *original, char *add)
{
   while(*original)
      original++;

   while(*add)
   {
      *original = *add;
      add++;
      original++;
   }
   *original = '\0';
}
void send_response(struct http_request *req,char *response,char *response_body){
    //response sending code
    if (req->keepalive == 1) {
        if(req->minor_version==1)
        {
            //char *response_body="<html><body><H1>Hello World</H1></body></html>";
            char str_length[12];
            //response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,&str_length[0]);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);
        }else{

            char str_length[12];
            //response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.0 200 OK\nConnection: keep-alive\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,str_length);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);
        }

    }
    else{
        if(req->minor_version==1)
        {
            char str_length[12];
            sprintf(str_length, "%d", str_len(response_body));
            //response[0]='\0';
            concatenate_string(response,"HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,&str_length[0]);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);

        }else{
            char str_length[12];
            //response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.0 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,&str_length[0]);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);


        }

        close(req->socket_id);
    }


    free(req);
    free(response);
    free(response_body);
}
void network_thread_function(){
    int number_of_ready_events;
    struct epoll_event *events;
    int max_events_to_stop_waiting=200;
    int epoll_time_wait=50;
    events = malloc (sizeof (struct epoll_event)*max_events_to_stop_waiting); //epoll event allocation for multiplexing
    int i=0,j=0; //iterators
    char response_buffer[1024*1024];
    char *response=response_buffer;
    response_buffer[0]='\0';
    char read_buffer[1024*1024];
    //char body[512*1024];
    char header_key[100],header_value[100];
    int minor_version;
    //struct phr_header headers[100];
    size_t read_buffer_length = 0, method_length, path_length, num_headers;
    ssize_t received_bytes; char *method, *path;
    int body_index=0; int headerover=0;
    int header_end=received_bytes-4;
    struct http_request *req;

    while(1){
        number_of_ready_events = epoll_wait (epfd, events, max_events_to_stop_waiting, epoll_time_wait);
        for (i = 0; i < number_of_ready_events; i++) {

            req=malloc(sizeof(struct http_request));
            req->socket_id=events[i].data.fd;
            received_bytes=recv(events[i].data.fd, read_buffer ,1024*1024,MSG_DONTWAIT);


            //splitting header and body;
            header_end=received_bytes-4;//header_end is an index to use in read_buffer. 4 indicates \r\n\r\n in http request to split body and header.
            body_index=0;headerover=0;//body array iterator and headerover is a flag to use inside below loop
            for(j=0;j<header_end;j++){
                if(headerover==0){
                    if(read_buffer[j]=='\r' && read_buffer[j+1]=='\n' && read_buffer[j+2]=='\r' && read_buffer[j+3]=='\n'){
                        j=j+4;header_end=received_bytes; headerover=1;
                    }
                }
                if (headerover==1) {
                    req->body[body_index++]=read_buffer[j];
                }
            }
            //end of splitting header and body. now body holds full data payload in http request.


            //parsing headers
            num_headers = sizeof(req->headers) / sizeof(req->headers[0]);
            phr_parse_request(read_buffer, received_bytes, &method, &method_length, &path, &path_length,&req->minor_version, req->headers, &num_headers, 0);
            //end of parsing headers


            // getting keepalive or not
            //int keepalive=0; // 1 indicates keepalive connection
            if(req->minor_version==1){req->keepalive=1;}
            for (j = 0; j != num_headers; j++) {
                //    printf("%.*s: %.*s\n", (int)headers[k].name_len, headers[k].name,(int)headers[k].value_len, headers[k].value);
                sprintf(header_key,"%.*s", (int)req->headers[j].name_len, req->headers[j].name);
                sprintf(header_value,"%.*s", (int)req->headers[j].value_len, req->headers[j].value);
                if (strcmp(header_key,"Connection")==0) {
                    if (header_value[0]=='K'||header_value[1]=='K'||header_value[0]=='k'||header_value[1]=='k') {
                        req->keepalive=1;
                    }
                }
            }
            // found type of connection keepalive or not



            //response sending code
            if (req->keepalive != 1){
                epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
            }
            response_buffer[0]='\0';
            send_response(req,&response_buffer[0],"hello");
        }
    }
}
//
//
//
//
// void network_thread_function(){
//         int nr_events;
//         struct epoll_event *events;
//
//   events = malloc (sizeof (struct epoll_event)*200);
//   int ret,i,received;
//   char buf[24*4096];
//   char body[1024*1024];
//   char str_key[100],str_value[100];
//   int pret, minor_version;
//   struct phr_header headers[100];
//   size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
//   ssize_t rret; char *method, *path;
//   int bindex=0; int headerover=0;
//   int j=0;  int end=rret-4;
//   while(1){
//
//     nr_events = epoll_wait (epfd, events, 200, 50);
//     for (i = 0; i < nr_events; i++) {
//
//      bindex=0;  headerover=0;
//
//     rret=recv(events[i].data.fd, buf ,24*4096,MSG_DONTWAIT);
//
//      j=0;
//      end=rret-4;
//         while(j<end){
//           if(headerover==0){
//             if(buf[j]=='\r' && buf[j+1]=='\n' && buf[j+2]=='\r' && buf[j+3]=='\n'){
//               j=j+4;end=rret+4; headerover=1;
//             }
//           }
//           if (headerover==1) {
//             body[bindex++]=buf[j];
//           }
//           j++;
//         }
//
//
//     buflen += rret;
//
//     num_headers = sizeof(headers) / sizeof(headers[0]);
//
//     pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
//                              &minor_version, headers, &num_headers, 0);
//
//   //  printf("minor version %d",minor_version );
// if(headerover==1){
//   for(j=0;j<=bindex;j++){
//     //printf("%c",body[j]);
//     //printf("%d\n",(int)sizeof(body));
//   }
// }
//
// int keepalive=0;
// if(minor_version==1)
//     {keepalive=1;}
//     int k=0;
// for (k = 0; k != num_headers; ++k) {
// //    printf("%.*s: %.*s\n", (int)headers[k].name_len, headers[k].name,(int)headers[k].value_len, headers[k].value);
// sprintf(str_key,"%.*s", (int)headers[k].name_len, headers[k].name);
// sprintf(str_value,"%.*s", (int)headers[k].value_len, headers[k].value);
//   if (strcmp(str_key,"Connection")==0) {
//     if (str_value[0]=='K'||str_value[1]=='K'||str_value[0]=='k'||str_value[1]=='k') {
//       keepalive=1;
//     }
//   }
//          }
//
// if (keepalive == 1) {
//   if(minor_version==1)
//       {
//  send(events[i].data.fd,"HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",106,MSG_DONTWAIT|MSG_NOSIGNAL);
// }else{
//   send(events[i].data.fd,"HTTP/1.0 200 OK\nConnection: keep-alive\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",129,MSG_DONTWAIT|MSG_NOSIGNAL);
// }
//
// }else{
//   if(minor_version==1)
//       {
//           send(events[i].data.fd,"HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",124,MSG_DONTWAIT|MSG_NOSIGNAL);
//       }else{
//           send(events[i].data.fd,"HTTP/1.0 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length: 46\n\n<html><body><H1>Hello World</H1></body></html>",124,MSG_DONTWAIT|MSG_NOSIGNAL);
//
//       }
//
//
//        ret = epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
//       close(events[i].data.fd);
//
//
// }
//
//
//
//  }
// }
// }
//

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
