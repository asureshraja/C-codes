#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
void my();
void my(){
    std::cout << "/* message */" << std::endl;
}
int main(){
    boost::asio::io_service ioService;
    boost::thread_group threadpool;


    /*
     * This will start the ioService processing loop. All tasks
     * assigned with ioService.post() will start executing.
     */
    boost::asio::io_service::work work(ioService);

    /*
     * This will add 2 threads to the thread pool. (You could just put it in a for loop)
     */
    threadpool.create_thread(
        boost::bind(&boost::asio::io_service::run, &ioService)
    );
    threadpool.create_thread(
        boost::bind(&boost::asio::io_service::run, &ioService)
    );

    /*
     * This will assign tasks to the thread pool.
     * More about boost::bind: "http://www.boost.org/doc/libs/1_54_0/libs/bind/bind.html#with_functions"
     */
    ioService.post(boost::bind(my, "Hello World!"));


    /*
     * This will stop the ioService processing loop. Any tasks
     * you add behind this point will not execute.
    */
    ioService.stop();

    /*
     * Will wait till all the treads in the thread pool are finished with
     * their assigned tasks and 'join' them. Just assume the threads inside
     * the threadpool will be destroyed by this method.
     */
    threadpool.join_all();
}
