hello: ./src/hello/hello.c ./src/hello/hellofunc.c
gcc -o ./build/hello ./src/hello/hello.c ./src/hello/hellofunc.c -I./src/hello/.
