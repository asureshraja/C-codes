#include <iostream>
#include <string>
#include "concurrentqueue.h"
#include <string.h>
moodycamel::ConcurrentQueue<struct worker_args *> q;
extern "C" void enqueue(struct worker_args *arg){
        q.enqueue(arg);
}
extern "C" struct worker_args * dequeue(){
        struct worker_args *item;
        bool retval = q.try_dequeue(item);
        if(!retval){return NULL;}
        return item;
}
