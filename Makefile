.PHONY: all clean

splexer.o: splexer.c
	clang -Wall -Wextra -ggdb -fPIC -c splexer.c

libsplexer.so: splexer.o
	mkdir -p build/
	gcc -shared splexer.o -o build/libsplexer.so

libsplexer.a: splexer.o
	mkdir -p build/
	ar rcs build/libsplexer.a splexer.o

main: main.c libsplexer.so libsplexer.a
	clang -Wall -Wextra -ggdb -o main main.c -L./build -I. -l:libsplexer.so

valgrind: main
	LD_LIBRARY_PATH=./build valgrind --leak-check=full --track-origins=yes ./main

clean:
	rm -rf *.o
