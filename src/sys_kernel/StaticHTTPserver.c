#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE
#include <netinet/in.h>
#include <stdio.h>
#include <netinet/tcp.h>
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
#include "tinydir.h"
#include <error.h>
#include <dirent.h>
#include "server.h"
#include <fcntl.h>
#include "thpool.h"
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <sched.h>

/*
Experimental c server code
g++ -std=c++11 -c -fPIC -Wall queueapi.cpp  -o queueapi.o
g++ -shared -o libq.so queueapi.o
gcc -L./ -Wall HttpServer.c http_parser.c thpool.c -o server -lq -w -lpthread -g
*/
char* concatenate( char* dest, char* src );
int str_len(const char *str);
int server_socket;int _eventfd;
int epfd;   int  new_socket;
extern struct worker_args * dequeue2();
extern  void enqueue2(struct worker_args *data);
extern struct worker_args * dequeue();
extern  void enqueue(struct worker_args *data);
threadpool thpool;


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
    char body[512*512];
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
        int file_to_serve;
void work(struct worker_args *args){
    // printf("%s\n","inside work" );
    stick_this_thread_to_core(-1);


    char *response_buffer=malloc(sizeof(char)*2048*8);
    response_buffer[0]='\0';
    args->response_buffer=response_buffer;//remaining will be filled by send
    args->response_body=malloc(sizeof(1024));
    lseek(file_to_serve,(off_t)0, SEEK_SET);

    int ret = read(file_to_serve, args->response_body,1024);


    // printf("%d\n",ret );
    //enqueue(args);
    //eventfd_write(_eventfd, 1);
    // printf("%s\n","written" );
    send_response(args->req, args->response_buffer, args->response_body);

}
int stick_this_thread_to_core(int core_id) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core_id < 0 || core_id >= num_cores)
      return -1;
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void network_thread_function(){
    static int core_id=-1;
    //core_id=core_id+1;
    stick_this_thread_to_core(core_id);
    int number_of_ready_events;
    struct epoll_event *events;
    int max_events_to_stop_waiting=10;
    int epoll_time_wait=100;
    events = malloc (sizeof (struct epoll_event)*max_events_to_stop_waiting); //epoll event allocation for multiplexing
    int i=0,j=0; //iterators
    //char *response_buffer;
    char read_buffer[512*512];
    //char body[512*1024];
    char header_key[100],header_value[100];
    int minor_version;
    struct phr_header headers[100];
    size_t read_buffer_length = 0, method_length, path_length, num_headers;
    ssize_t received_bytes; char *method, *path;
    int body_index=0; int headerover=0;
    int header_end=received_bytes-4;
    struct http_request *req;
    struct worker_args *arg=NULL;
    while(1){
        number_of_ready_events = epoll_wait (epfd, events, max_events_to_stop_waiting, epoll_time_wait);

        for (i = 0; i < number_of_ready_events; i++) {
            //printf("%s\n","network thread" );
            //printf("i val %d\n", i);
            //printf("%d\n", events[i].data.fd);



            //read_buffer=malloc(sizeof(char)*512*512);
            //memset(read_buffer, '\0', 512*512);
            req=malloc(sizeof(struct http_request));
            req->socket_id=events[i].data.fd;
            read_buffer[0]='\0';
            received_bytes=recv(events[i].data.fd, read_buffer ,512*512,MSG_DONTWAIT);
            read_buffer[received_bytes]='\0';

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
            //printf("%s\n",read_buffer );
            for (j = 0; j != num_headers; j++) {
                    //printf("%.*s: %.*s\n", (int)headers[j].name_len, headers[j].name,(int)headers[j].value_len, headers[j].value);
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

            struct worker_args *args=malloc(sizeof(struct worker_args));
            args->req=req;
            work(args);
            //send_response(arg->req, arg->response_buffer, arg->response_body);



        }


    }
}

int main() {
    // tinydir_dir dir;
    //     tinydir_open(&dir, "/home/suresh/codes/sample/");
    //     printf("%zd\n",dir.n_files );
    //     while (dir.has_next)
    //     {
    //         tinydir_file file;
    //         tinydir_readfile(&dir, &file);
    //
    //         printf("%s", file.name);
    //         if (file.is_dir)
    //         {
    //             ////////////////////////
    //             tinydir_dir dir1;
    //             tinydir_open(&dir1,file.path );
    //
    //             ///////////////////////
    //             printf("/");
    //         }
    //         printf("\n");
    //
    //         tinydir_next(&dir);
    //     }
    //
    //     tinydir_close(&dir);


            file_to_serve=open ("/home/suresh/Desktop/test.html", O_RDONLY);
   socklen_t addrlen,clientaddresslen;
   int bufsize = 1024;
   char *buffer = malloc(bufsize);
   struct sockaddr_in address,clientaddress;
   memset((void*)&address,(int)'\0',sizeof(address));
   memset((void*)&clientaddress,(int)'\0',sizeof(clientaddress));

   if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
      printf("The socket was created\n");
   }
   int ii=1;
   setsockopt( server_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&ii, sizeof(ii));
   setsockopt( server_socket, IPPROTO_TCP, TCP_QUICKACK, (void *)&ii, sizeof(ii));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(5000);

   if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
      printf("Binding Socket\n");
   }
   int optval = 1;
   setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR|SO_LINGER, &optval, sizeof(optval));

   epfd=epoll_create(1000-1);
   struct epoll_event *events;
   events = malloc (sizeof (struct epoll_event)*2);
   int ret,i;

   _eventfd = eventfd(0, EFD_NONBLOCK);
   struct epoll_event evnt;
   evnt.data.fd = _eventfd;
   evnt.events = EPOLLIN | EPOLLET;
   epoll_ctl(epfd, EPOLL_CTL_ADD, _eventfd, &evnt);

   //thpool = thpool_init(2);
   pthread_t threads[4];
   for (i = 0; i < 4; i++) {
     pthread_create( &threads[i], NULL, &network_thread_function, NULL);
   }
   while (1) {
      if (listen(server_socket, 10000) < 0) {
         perror("server: listen");
         //exit(1);
      }
clientaddresslen=sizeof(clientaddress);
      if ((new_socket = accept4(server_socket, (struct sockaddr *) &clientaddress, &clientaddresslen,SOCK_NONBLOCK)) < 0) {
         perror("server: accept");
         //exit(1);
      }

      if (new_socket > 0){
          setsockopt( new_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&ii, sizeof(ii));
          setsockopt( new_socket, IPPROTO_TCP, TCP_QUICKACK, (void *)&ii, sizeof(ii));
        events[0].data.fd = new_socket;
        events[0].events = EPOLLIN | EPOLLET;
        ret = epoll_ctl (epfd, EPOLL_CTL_ADD, events[0].data.fd, &events[0]);
      }

   }
   close(server_socket);
   return 0;
}
