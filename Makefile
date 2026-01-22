splexer.o: splexer.c
	clang -Wall -Wextra -ggdb -fPIC -c splexer.c

libsplexer.so: splexer.o
	mkdir -p build/
	gcc -shared splexer.o -o build/libsplexer.so

libsplexer.a: splexer.o
	mkdir -p build/
	ar rcs build/libsplexer.a splexer.o

main: main.c libsplexer.a
	clang -Wall -Wextra -ggdb -static -o main main.c -L./build -I. -lsplexer


valgrind: main
	valgrind --leak-check=full ./main
