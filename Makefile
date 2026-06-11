.PHONY: all clean

BUILDDIR := ./build
OBJDIR := ./obj

%.o: %.c
	mkdir -p $(OBJDIR)
	clang -Wall -Wextra -ggdb -fPIC -o $(OBJDIR)/$@ -c $<

lib%.so: %.o
	mkdir -p $(BUILDDIR)
	gcc -shared $(OBJDIR)/$< -o $(BUILDDIR)/$@

lib%.a: %.o
	mkdir -p $(BUILDDIR)
	ar rcs $(BUILDDIR)/$@ $(OBJDIR)/$<

main: main.c libsplexer.so libsplexer.a
	clang -Wall -Wextra -ggdb -o $@ $< -L$(BUILDDIR) -I. -l:libsplexer.so

valgrind: main
	LD_LIBRARY_PATH=./build valgrind --leak-check=full --track-origins=yes ./main

clean:
	rm -rf *.o
