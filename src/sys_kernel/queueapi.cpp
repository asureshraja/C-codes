#include <iostream>
#include <string>
#include "concurrentqueue.h"
#include <string.h>
#include "server.h"
moodycamel::ConcurrentQueue<struct worker_args *> q(1000000);
moodycamel::ConcurrentQueue<struct worker_args *> outq(1000000);
extern "C" void enqueue(struct worker_args *arg){
        while(q.try_enqueue(arg)==false){
            std::cout << "failing in q elem allocation" << std::endl;
        }

}
extern "C" struct worker_args * dequeue(){
        struct worker_args *item;
        bool retval = q.try_dequeue(item);
        if(!retval){return NULL;}
        return item;
}

extern "C" void enqueue2(struct worker_args *arg){
        while(outq.try_enqueue(arg)==false){
            std::cout << "failing in q elem allocation" << std::endl;
        }

}
extern "C" struct worker_args * dequeue2(){
        struct worker_args *item;
        bool retval = outq.try_dequeue(item);
        if(!retval){return NULL;}
        return item;
}
