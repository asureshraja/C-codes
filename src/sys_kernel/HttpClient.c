#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#define USERAGENT "THROWREQUEST"
int success=0;
int keep_alive=0;
struct timeval  tv1;
int prev_success=0;
int print_flag=0;
char *user_given_host="localhost";
char *user_given_filename="index.html";
int user_given_threads=1;
int user_given_connections=100;
int user_given_requests=100000;

struct request_timings{
  struct timeval tv1;
  double thread_start;
  double thread_time;
};
struct request_timings *rt_threads;
int temp=0;
int user_given_port=8055;
struct req_struct {
  char *host;
  int port;
  int connections;
  int num_of_requests;
  int threadID;
};
void *progress();

char *get_ip(char *host);
char *build_get_query(char *host, char *page);
int get_index_of_socket(int req_no,int num_of_conn,int total_req);
void *throw_requests(struct req_struct *req);

int main(int argc, char *argv[]){


  int c;
    while ((c=getopt(argc, argv, "n:t:c:h:p:f:kh"))!=-1) {
      switch (c) {
        case '?':
        printf( "throwrequests <options>\n"
        "  -n num         number of requests     (default: 1)\n"
        "  -t num         number of threads      (default: 1)\n"
        "  -c num         concurrent connections (default: 1)\n"
        "  -h hostname    host name (default: localhost)\n"
        "  -p num         port number (default: 80)\n"
        "  -f filename    file name to request (default: index.html)\n"
        "  -k             keep alive             (default: no)\n"
        "  -h             show this help\n"
        "\n"
        "example: httpress -n 10000 -c 100 -t 4 -k http://localhost:8080/index.html\n\n");
          return 0;
        case 'k':
          keep_alive=1;
          break;
        case 'h':
          user_given_host=malloc(sizeof(char)*(1000));
          strcpy(user_given_host,optarg);
          break;
        case 'f':
          user_given_filename=malloc(sizeof(char)*(1000));
          strcpy(user_given_filename,optarg);
          break;
        case 'n':
          user_given_requests=atoi(optarg);
          break;
        case 't':
          user_given_threads=atoi(optarg);
          break;
        case 'c':
          user_given_connections=atoi(optarg);
          break;
        case 'p':
          user_given_port=atoi(optarg);
          break;

      }
    }

  int num_threads=user_given_threads;

struct timeval  tv;
    struct req_struct *req;
    rt_threads=malloc(sizeof(struct request_timings)*num_threads);
    req = malloc(sizeof(struct req_struct)*num_threads);

    //int success = throw_requests(host, port, connections, num_of_requests);
    pthread_t threads[num_threads];
    //pthread_t progress_thread;
    int rc;
    long t;


    for(t=0; t<num_threads; t++){
      req[t].host=malloc(sizeof(char)*1000);
      strcpy(req[t].host,user_given_host);
      req[t].port=user_given_port;
      req[t].threadID=t;
      req[t].num_of_requests=user_given_requests/num_threads;
      req[t].connections=user_given_connections/num_threads;
    //  rc = pthread_create(&progress_thread, NULL, progress, NULL);
      printf("In main: creating thread %ld\n", t);
      rc = pthread_create(&threads[t], NULL, throw_requests, &req[t]);
      if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
      }

    }
    for(t=0; t<num_threads; t++){
      pthread_join(threads[t], NULL);
    }
    gettimeofday(&tv, NULL);


      double finalresult=0;
      for(t=0; t<num_threads; t++){
        //printf("%lf %d\n",rt_threads[t].thread_time,req[t].num_of_requests);
        finalresult=finalresult+(req[t].num_of_requests/(rt_threads[t].thread_time/1000));
      }
      printf("%d\n", (int)finalresult);

}

// void *progress(){
//   while(1){
//     if(success>temp){
//       temp=temp+(user_given_requests/10);
//       printf("%d finished\n",temp);
//     }
//     sleep(0.01)  ;
//   }
//
// }


void *throw_requests(struct req_struct *req){
  char *host = req->host;
  int port = req->port;
  int connections=req->connections;
  int num_of_requests=req->num_of_requests;
  char *get; //get request to send;
  int sock[connections];
  int i=0;int flags=0;
  //epoll declarations
  int epfd;
  epfd=epoll_create(connections+1);
  struct epoll_event *events;
  events = malloc (sizeof (struct epoll_event)*connections);
  int ret;int nr_events;
  //end of epoll declarations
  //remote address block
  struct sockaddr_in *remote; //remote address structure
  remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *)); //allocating remote address
  remote->sin_family = AF_INET;
  inet_pton(AF_INET, get_ip(host), (void *)(&(remote->sin_addr.s_addr))); //setting ip
  remote->sin_port = htons(port); //setting port


  for (i = 0; i < connections; i++) {
    if((sock[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){ //creating tcp socket
      perror("Can't create TCP socket");
      exit(1);
    }
    // flags = fcntl(sock[i], F_GETFL, 0);
    // fcntl(sock[i], F_SETFL, flags | O_NONBLOCK);
    int optval=1;
    socklen_t optlen = sizeof(optval);

    if(setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &optval, optlen) < 0) {
       perror("setsockopt()");
       exit(EXIT_FAILURE);
    }
     if(setsockopt(sock[i], SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
     }
     events[i].data.fd = sock[i];
     events[i].events = EPOLLIN;
     ret = epoll_ctl (epfd, EPOLL_CTL_ADD , sock[i], &events[i]);
  }

  get = build_get_query(host, user_given_filename); //get request to send to remote
  char *tempget;int sent = 0; ret=0;
  tempget=get;int len=strlen(get);
  int progress=num_of_requests/10;



  gettimeofday(&rt_threads[req->threadID].tv1, NULL);
    rt_threads[req->threadID].thread_start =
         (rt_threads[req->threadID].tv1.tv_sec) * 1000 + (rt_threads[req->threadID].tv1.tv_usec) / 1000 ;



  for (i = 0; i < connections; i++) {
    if(connect(sock[i], (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){ //connecting to remote address and ataching to socket
      printf("%d\n",i );
       for (i = 0; i < connections; i++) {
          close(sock[i]);
        }
      exit(1);
    }

    //following block sends get string headers to remote address
    sent = 0;ret=0;
    while(sent < len)
    {
      ret = send(sock[i], get+sent, len-sent, 0);
      if(ret == -1){
        perror("Can't send query");
        exit(1);
      }
      sent += ret;
    }
    get=tempget;
    // end of send block
  }

  gettimeofday(&rt_threads[req->threadID].tv1, NULL);
  double tm=((rt_threads[req->threadID].tv1.tv_sec) * 1000 + (rt_threads[req->threadID].tv1.tv_usec) / 1000);
  rt_threads[req->threadID].thread_time=rt_threads[req->threadID].thread_time+(tm-rt_threads[req->threadID].thread_start);

  get=tempget;int tmpsock;
  char *buf;
  buf = (char *)malloc(sizeof(char)*1024);
  int j=0;  gettimeofday(&rt_threads[req->threadID].tv1, NULL);
    rt_threads[req->threadID].thread_start =
         (rt_threads[req->threadID].tv1.tv_sec) * 1000 + (rt_threads[req->threadID].tv1.tv_usec) / 1000 ; // convert tv_sec & tv_usec to mill
  for (; i < num_of_requests;i=i) {
    nr_events = epoll_wait (epfd, events,connections, -1);
      for (j=0;j < nr_events && i < num_of_requests; i++,j++) {
        // if(i%progress==0 && i!=0)printf("%d completed\n",i );

        recv(events[j].data.fd, buf, 1024*1024, 0);
          if(keep_alive==0)
         {
           ret = epoll_ctl (epfd, EPOLL_CTL_DEL,events[j].data.fd, &events[j]);
         }


          gettimeofday(&rt_threads[req->threadID].tv1, NULL);
          double tm=((rt_threads[req->threadID].tv1.tv_sec) * 1000 + (rt_threads[req->threadID].tv1.tv_usec) / 1000);
          rt_threads[req->threadID].thread_time=rt_threads[req->threadID].thread_time+(tm-rt_threads[req->threadID].thread_start);

        success++;
        if(success>temp){
          temp=temp+(user_given_requests/10);
          printf("%d finished\n",temp);
        }
         //printf("%d %s\n",success ,buf);
         if(keep_alive==0)
        {
            close(events[j].data.fd);


          ///////////////////////////
          if((tmpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){ //creating tcp socket
            perror("Can't create TCP socket");
            exit(1);
          }
          // flags = fcntl(sock[i], F_GETFL, 0);
          // fcntl(sock[i], F_SETFL, flags | O_NONBLOCK);
          int optval=1;
          socklen_t optlen = sizeof(optval);

          if(setsockopt(tmpsock, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT|SO_KEEPALIVE, (char*)&optval, sizeof(optval)) < 0) {
             perror("setsockopt()");
             exit(EXIT_FAILURE);
          }

           events[j].data.fd = tmpsock;
           events[j].events = EPOLLIN;
           ret = epoll_ctl (epfd, EPOLL_CTL_ADD, tmpsock, &events[j]);
       }
        ///////////////////////////
        gettimeofday(&rt_threads[req->threadID].tv1, NULL);
        rt_threads[req->threadID].thread_start =
             (rt_threads[req->threadID].tv1.tv_sec) * 1000 + (rt_threads[req->threadID].tv1.tv_usec) / 1000 ; // convert tv_sec & tv_usec to mill
             if(keep_alive==0)
            {
              if(connect(events[j].data.fd, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){ //connecting to remote address and ataching to socket
                perror("Could not connect");
                exit(1);
              }
            }
      //  following block sends get string headers to remote address
        sent = 0;ret=0;
        while(sent < len)
        {
          ret = send(events[j].data.fd, get+sent, len-sent, 0);
          if(ret == -1){
            perror("Can't send query");
            exit(1);
          }
          sent += ret;
        }
        get=tempget;
        // end of send block
    }
  }
  // printf("finished %d\n", success);
  // printf("%d thread finished.\n",(int)pthread_self() );
  close(epfd);
  free(buf);
  free(events);
  free(host);
  free(remote);
}



int get_index_of_socket(int req_no,int num_of_conn,int total_req){
  int spliter=total_req/num_of_conn;
  return (req_no/spliter);
}
char *build_get_query(char *host, char *page)
{
  char *query;
  char *getpage = page;
  char *tpl = "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
  if(getpage[0] == '/'){
    getpage = getpage + 1;
    fprintf(stderr,"Removing leading \"/\", converting %s to %s\n", page, getpage);
  }
  // -5 is to consider the %s %s %s in tpl and the ending \0
  query = (char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tpl)-5);
  sprintf(query, tpl, getpage, host, USERAGENT);
  return query;
}
char *get_ip(char *host)
{
  struct hostent *hent;
  int iplen = 15; //XXX.XXX.XXX.XXX
  char *ip = (char *)malloc(iplen+1);
  memset(ip, 0, iplen+1);
  if((hent = gethostbyname(host)) == NULL)
  {
    herror("Can't get IP");
    exit(1);
  }
  if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
  {
    perror("Can't resolve host");
    exit(1);
  }
  return ip;
}
