main: main.c
	clang -Wall -Wextra -ggdb -o main main.c


valgrind: main
	valgrind --leak-check=full ./main
