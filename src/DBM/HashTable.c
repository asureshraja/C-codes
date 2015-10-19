#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

int create_file(char* file_name){
    int fd;
    fd = open(file_name, O_RDWR|O_CREAT, (mode_t)0600);
    if(fd==-1){
        perror("file creation error");
        exit(EXIT_FAILURE);
    }
    return fd;
}
int open_file(char* file_name){
    int fd;
    fd = open(file_name, O_RDWR, (mode_t)0600);
    if(fd==-1){
        perror("file opening error");
        exit(EXIT_FAILURE);
    }
    return fd;
}
char* create_mmap_for_new_file(int fd,int initial_page_size){
    char *data;
    if (lseek(fd, initial_page_size, SEEK_SET) == -1)
    {
        close(fd);
        perror("Error calling lseek() to 'stretch' the file");
        exit(EXIT_FAILURE);
    }
    write(fd,"-", 1);//writing as last char
    data = mmap((caddr_t)0, initial_page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
    0);
    if (data == MAP_FAILED)
            handle_error("mmap");
    return data;
}
char* create_mmap(int fd,int offset,int page_size){
    char *data = mmap((caddr_t)0, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
    offset);
    if (data == MAP_FAILED)
            handle_error("mmap");

    return data;
}
int get_file_length(int fd){
    struct stat fileInfo = {0};
    if (fstat(fd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }
    if (fileInfo.st_size == 0)
    {
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        exit(EXIT_FAILURE);
    }
    return fileInfo.st_size;
}

void delete_mmap(char *data,int mapped_size){
    munmap (data, mapped_size);
}

int main(){
    char *data=NULL;
    int fd=open_file("/home/suresh/Desktop/test.db");
    //create_mmap_for_new_file(create_file("/home/suresh/Desktop/test.db"),getpagesize());
    data=create_mmap(fd,0,10000);

    //data="hello";
    printf("%c\n",data[0]);
    delete_mmap(data, getpagesize());
}
