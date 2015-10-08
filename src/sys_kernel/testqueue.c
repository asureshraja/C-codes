#include <stdio.h>
extern  char * dequeue();
extern  void enqueue(char *data);
int main(){
    enqueue("sDfsd");
    printf("%s\ndd", dequeue());
    if(dequeue()==NULL){
        printf("%s\n","dsfsd");
    }
    return 0;
}
