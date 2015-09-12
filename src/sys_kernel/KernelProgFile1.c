/*
 * File:   main.c
 * Author: suresh
 *
 * Created on 9 September, 2015, 10:35 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
/* open,read and seek
 *
 */
int main(int argc, char** argv) {
    int fd;
    fd = open ("/home/suresh/Desktop/test.txt", O_RDWR);
    if (fd == -1)
        printf("error opening file.");
    char *str=malloc(sizeof(1000)); //allotted with max length of file assumed as 1000 bytes
    char *tmp = str;
    ssize_t ret=0;
    ssize_t count=0;
    printf("length of file is %d\n",(int) lseek(fd,(off_t)0, SEEK_END));
    lseek(fd,(off_t)0, SEEK_SET);
    //file inmemory read
    while ((ret = read (fd,str, 10)) != 0) {
        // printf("ret val %d\n",(int)ret );
        if (ret == -1) {
           if (errno == EAGAIN)
               continue;
          perror("read");
           break;
        }
        str+=ret;
    }
    str=tmp;
    printf(" data %s\n", str );
    free(str);
    close(fd);
    return (EXIT_SUCCESS);
}
