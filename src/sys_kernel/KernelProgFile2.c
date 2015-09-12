#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
int main(void) {
    fd_set read_fd_set;
    struct timeval time_value;
    int retval;
    char *str=malloc(sizeof(char)*10);
    FD_ZERO(&read_fd_set);
    FD_SET(0, &read_fd_set); //adding stdin to read_fd_set
    time_value.tv_sec = 5;
    time_value.tv_usec = 0;
    retval = select(1, &read_fd_set, NULL, NULL, &time_value); // setting read fd set and others as NULL

    if (retval == -1)
        perror("select()");
    else if (retval){
        printf("reading value");
          read(0, str, 10);
    }
    else
        printf("No data within five seconds.\n");

        printf("%d\n",(int)time_value.tv_sec);

    return 0;
}
