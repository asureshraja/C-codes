#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
char* concatenate( char* dest, char* src );
struct data{
    int key_len;
    int value_len;
    char *key;
    char *value;
};
struct ptr_data{
    off_t rec_offset;
};
//map of unsigned int key_hash and ptr_data offset
struct idx_data{
    unsigned int key_hash;
};
int create_file(char* file_name){
    int fd;
    fd = open(file_name, O_RDWR|O_CREAT, (mode_t)0600);
    if(fd==-1){
        perror("file creation error");
        exit(EXIT_FAILURE);
    }
    return fd;
}
int open_file(char* file_name,int flag){
    int fd;
    fd = open(file_name, flag, (mode_t)0600);
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
    if (data == MAP_FAILED) {
    	close(fd);
    	perror("Error mmapping the file");
    	exit(EXIT_FAILURE);
    }
    return data;
}
char* create_mmap(int fd,int offset,int page_size){
    char *data = mmap((caddr_t)0, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
    offset);
    if (data == MAP_FAILED){
        close(fd);
    	perror("Error mmapping the file");
    	exit(EXIT_FAILURE);
    }

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
void delete_mmap(char *data,int mapped_size){
    munmap (data, mapped_size);
}

//key%%value%%
long long int add_data_to_file(int fd,struct data *data_to_add);
long long int add_data_to_file(int fd,struct data *data_to_add){ //need to optimize later
    int write_pos=lseek(fd, 0, SEEK_END);
    int total_size=data_to_add->key_len+data_to_add->value_len+4;
    char buffer[total_size+10];
    buffer[0]='\0';
    concatenate(buffer, data_to_add->key);
    concatenate(buffer, "%%");
    concatenate(buffer, data_to_add->value);
    concatenate(buffer, "%%");
    pwrite(fd, buffer, total_size,write_pos);
    //char buf[1024];
    //int nr=pread(fd, buf, 1024,0);
    //printf("total size expected %d and read %d\n",total_size,nr );
    //printf("after write %s\n",buf );
    return write_pos;
}
struct data * read_data_from_file(int fd,off_t offset,struct data *data_to_add);
struct data * read_data_from_file(int fd,off_t offset,struct data *data_to_add){
    char buffer[1024];
    data_to_add = malloc(sizeof(data_to_add));
    pread(fd, buffer, sizeof(buffer),offset);
    int separator_count=0,i=0;
    int key_for_zero_value_for_one=0;
    data_to_add->key_len=0;
    data_to_add->value_len=0;
    data_to_add->key = malloc(sizeof(char)*1024*512);
    data_to_add->value = malloc(sizeof(char)*1024*512);
    int kstart=0;int kend=0;int vstart=0;int vend=0;
    while (separator_count!=4) {
        if(i==1024){
            read(fd, buffer, offset+1024);i=0;
        }
        if (separator_count<2 && buffer[i]!='%') {
            data_to_add->key[kend]=buffer[i];//printf("%c\n",buffer[i] );
            kend=kend+1;

        }
        if (separator_count>=2 && buffer[i]!='%') {
            data_to_add->value[vend]=buffer[i];
            vend=vend+1;
        }
        if(buffer[i]=='%'){
            //printf("%c\n",buffer[i] );
            separator_count=separator_count+1;
            if (separator_count==2) {
                key_for_zero_value_for_one=1;
                data_to_add->key_len+=kend-kstart;
                data_to_add->key[kend]='\0';
                //data_to_add->key = (char *)realloc(data_to_add->key,sizeof(char)*data_to_add->key_len);//realloc it
            }
            if (separator_count==4) {
                key_for_zero_value_for_one=1;
                data_to_add->value_len+=vend-vstart;
                data_to_add->key[vend]='\0';
                //data_to_add->value = (char *)realloc(data_to_add->key,sizeof(char)*data_to_add->value_len);//realloc it
                break;
            }
        }
        i=i+1;
    }
    return data_to_add;
}
int main(){
    signed long int ps = sysconf(_SC_PAGESIZE);
    signed long int pn = sysconf(_SC_AVPHYS_PAGES);
    unsigned long long avail_mem = ps * pn;

// first time code
    // char *data=NULL;
    // int fd=create_file("/home/suresh/Desktop/test.db");
    // data=create_mmap_for_new_file(fd,getpagesize());
    // delete_mmap(data, getpagesize());
    // close(fd);
// end of first time code

//mapping existing file code
    // char *data=NULL;
    // int fd=open_file("/home/suresh/Desktop/test.db",O_APPEND);
    // int file_length=get_file_length(fd);
    // int paged_size;
    // if(file_length>=avail_mem){
    //     paged_size=avail_mem-ps;
    //     data=create_mmap(fd,0,avail_mem-ps);
    // }else{
    //     paged_size=file_length;
    //     data=create_mmap(fd,0,file_length);
    // }
    //
    // printf("%c\n",data[1]);
    //delete_mmap(data, paged_size);
//mapping existing file code end

//writing to db file
    // int fd=open_file("/home/suresh/Desktop/test",O_RDWR|O_CREAT);
    // struct data *one=malloc(sizeof(struct data));
    // one->key="qwer";
    // one->key_len=4;
    // one->value="qwertdy";
    // one->value_len=7;
    // off_t offset=add_data_to_file(fd,one);
    // struct data *two;
    //
    // two=read_data_from_file(fd, offset, two);
    // printf("%jd\n", offset);
    // printf("%d\n",two->key_len );
    // printf("%s\n",two->key );
    // printf("%d\n",two->value_len );
    // printf("%s\n",two->value );
    // free(one);
    // free(two);
//writing to db file end
    close(fd);




}
