# C-codes
my c codes


code to compile http server 

cd src/sys_kernel

g++ -std=c++11 -c -fPIC -Wall queueapi.cpp  -o queueapi.o
g++ -shared -o libq.so queueapi.o
gcc -L./ -Wall HttpServer.c http_parser.c thpool.c -o server -lq -w -lpthread -g

make sure you have "src/sys_kernel" in $LD_LIBRARY_PATH


