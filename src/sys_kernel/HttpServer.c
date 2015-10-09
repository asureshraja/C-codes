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
#include "thread_pool.h"
#include "server.h"
#include <sys/eventfd.h>
/*
Experimental c code reading data on http body and sending some text as response
 gcc -L./ -Wall HttpServer.c http_parser.c -o server -lq -w -lpthread
TODO
1. Need to remove this threadpool and add someother its based on eventfd with epoll not working fast because of context switching happening in epoll_wait in work().
*/
extern struct worker_args * dequeue();
extern  void enqueue(struct worker_args *data);
void work();void pool_check();struct worker_args * get_one_from_queue();
void add_to_queue(struct worker_args *data);
char* concatenate( char* dest, char* src );
int str_len(const char *str);
int server_socket;
int epfd;   int  new_socket;int worker_epfd;
   void *pool;
   int epolleventfd, _eventfd;
    static int addedtoq=0,removedfromq=0;
struct worker_args * get_one_from_queue(){
    return dequeue();
}
void add_to_queue(struct worker_args *data){
    addedtoq++;
    //printf("a%d\n",addedtoq);
    enqueue(data);
    eventfd_write(_eventfd, 1);
}
void signal_callback_handler(int signum){
    eventfd_write(_eventfd, 1);
        //printf("Caught signal SIGPIPE %d\n",signum);
}
int init()
{
   epolleventfd = epoll_create(10);
   _eventfd = eventfd(0, EFD_NONBLOCK);
   struct epoll_event *evnt=malloc(sizeof( struct epoll_event));
   evnt->data.fd = _eventfd;
   evnt->events =  EPOLLIN | EPOLLET;
   epoll_ctl(epolleventfd, EPOLL_CTL_ADD, _eventfd, evnt);
}

void work()
{
    static const int EVENTS = 20;
    struct epoll_event *events=malloc(sizeof(struct epoll_event)*EVENTS);
    static int j=0;int i;
    while (1) {
        int count = epoll_wait(epolleventfd, events, EVENTS, 10);
        //printf("%d %d\n",addedtoq,removedfromq);
        if (count>0 || addedtoq>removedfromq) {
            eventfd_t val;
            eventfd_read(_eventfd,&val);
            pool_check();
        }

    }
}


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
    //printf("%s\n","inside send_response");
    //printf("%d\n",req->socket_id);

    if (req->keepalive == 1) {
        if(req->minor_version==1)
        {
            //char *response_body="<html><body><H1>Hello World</H1></body></html>";
            char str_length[12];
            response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,str_length);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            // printf("%s\n",response );//MSG_DONTWAIT|MSG_NOSIGNAL
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);
        }else{

            char str_length[12];
            response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.0 200 OK\nConnection: keep-alive\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,str_length);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            //printf("%s\n",response );
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);
        }

    }
    else{
        if(req->minor_version==1)
        {
            char str_length[12];
            response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,&str_length[0]);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            // printf("%s\n",response );
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);

        }else{
            char str_length[12];
            response[0]='\0';
            sprintf(str_length, "%d", str_len(response_body));
            concatenate_string(response,"HTTP/1.0 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length:");
            concatenate_string(response,&str_length[0]);
            concatenate_string(response,"\n\n");
            concatenate_string(response,response_body);
            // printf("%s\n",response );
            send(req->socket_id,response,str_len(response),MSG_DONTWAIT|MSG_NOSIGNAL);
        }
        close(req->socket_id);
    }
    free(req);
    free(response_body);
}
void pool_check(){
    //while(1){
        struct worker_args *arg=NULL;
        if((arg=get_one_from_queue())!=NULL){
            arg->response_body="hello";
            arg->response_buffer=malloc(sizeof(char)*512*1024);
            arg->response_buffer[0]='\0';
            //printf("%s\n",arg->response_body );
            send_response(arg->req,arg->response_buffer,arg->response_body);
            free(arg->response_buffer);
            free(arg);
            //printf("g%d\n",removedfromq++);
        }

    //}

}

void network_thread_function(){
    int number_of_ready_events;
    struct epoll_event *events;
    int max_events_to_stop_waiting=200;
    int epoll_time_wait=50;ssize_t received_bytes;
    events = malloc (sizeof (struct epoll_event)*max_events_to_stop_waiting); //epoll event allocation for multiplexing
    int i=0,j=0; //iterators

    char read_buffer[1024*1024];
    //char body[512*1024];
    char header_key[100],header_value[100];
    int minor_version;
    //struct phr_header headers[100];
    size_t read_buffer_length = 0, method_length, path_length, num_headers;
    char *method, *path;
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
            struct worker_args *arg;
            arg=malloc(sizeof(struct worker_args));
            arg->req=req;
            //printf("%d\n",arg->req->keepalive );
            //arg->response_buffer=response_buffer;

            if (req->keepalive != 1){
                epoll_ctl (epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
            }
            add_to_queue(arg);
            //enqueue(arg);
            //pool_check();
            //pool_check(arg);
            //submit_job(pool, &pool_check, arg);

            //printf("%d\n",req->keepalive );

            // response_buffer[0]='\0';
             //send_response(req,&response_buffer[0],"hello");
        }
    }
}

int main() {
    init();signal(SIGPIPE, signal_callback_handler);
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
   worker_epfd=epoll_create(10000);
   struct epoll_event *events;
   events = malloc (sizeof (struct epoll_event)*2);
   int ret,i;
   pthread_t threads[4];pthread_t workers[8];

   for (i = 0; i < 1; i++) {
     pthread_create( &threads[i], NULL, &network_thread_function, NULL);

   }
   for (i = 0; i < 1; i++) {
        pthread_create( &workers[i], NULL, &work, NULL);

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
