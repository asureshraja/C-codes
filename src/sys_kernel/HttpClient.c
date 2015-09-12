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
#define USERAGENT "THROWREQUEST"
int success=0;
int temp=0;
int prev_success=0;
int print_flag=0;
int user_given_threads=4;
int user_connection=100;
int user_given_requests=10000;
int user_given_port=80;
struct req_struct {
  char *host;
  int port;
  int connections;
  int num_of_requests;
};
void *progress();

char *get_ip(char *host);
char *build_get_query(char *host, char *page);
int get_index_of_socket(int req_no,int num_of_conn,int total_req);
void *throw_requests(struct req_struct *req);
int main(){
  int num_threads=user_given_threads;
    struct req_struct *req;
    req = malloc(sizeof(struct req_struct)*num_threads);

    //int success = throw_requests(host, port, connections, num_of_requests);
    pthread_t threads[num_threads];
    pthread_t counter;
    int rc;
    long t;
    struct timeval  tv;
    gettimeofday(&tv, NULL);

    double time_in_mill_start =
         (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to mill

    for(t=0; t<num_threads; t++){
      req[t].host="localhost";
      req[t].port=user_given_port;
      req[t].num_of_requests=user_given_requests/num_threads;
      req[t].connections=user_connection/num_threads;
      rc = pthread_create(&counter, NULL, progress, NULL);
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

    double time_in_mill_stop =
         (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to mill

         double val = time_in_mill_stop - time_in_mill_start;
      printf("%lf\n",(user_given_requests/val)*1000);


}


void *progress(){
  while (1) {
    temp=success;
    if((temp > (user_given_requests/user_connection)*print_flag) && prev_success!=temp){

      print_flag++;
      if(print_flag!=0)
      printf("%d finished\n",((user_given_requests/user_connection)*(print_flag-1)));sleep(1);

      prev_success=temp;
    }
    sleep(1);
  }


}
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
     ret = epoll_ctl (epfd, EPOLL_CTL_ADD, sock[i], &events[i]);
  }

  get = build_get_query(host, "index.html"); //get request to send to remote
  char *tempget;int sent = 0; ret=0;
  tempget=get;int len=strlen(get);
  int progress=num_of_requests/10;
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


  get=tempget;int tmpsock;
  char *buf;int j=0;
  for (i = 0; i < num_of_requests;) {
    nr_events = epoll_wait (epfd, events,connections, -1);
      for (j=0;j < nr_events && i < num_of_requests; i++) {
        // if(i%progress==0 && i!=0)printf("%d completed\n",i );
        recv(events[j].data.fd, buf, 1024, 0);
        ret = epoll_ctl (epfd, EPOLL_CTL_DEL,events[j].data.fd, &events[j]);
        success++;
        // printf("%d %s\n",success ,buf);
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
        ///////////////////////////

        if(connect(events[j].data.fd, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){ //connecting to remote address and ataching to socket
          perror("Could not connect");
          exit(1);
        }
        //following block sends get string headers to remote address
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
}



int get_index_of_socket(int req_no,int num_of_conn,int total_req){
  int spliter=total_req/num_of_conn;
  return (req_no/spliter);
}
char *build_get_query(char *host, char *page)
{
  char *query;
  char *getpage = page;
  char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
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
