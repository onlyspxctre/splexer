.PHONY: all clean

CC := clang
CFLAGS := -Wall -Wextra -std=c11 -fcolor-diagnostics

BUILDDIR := ./build
OBJDIR := ./obj

all: main

main: main.c $(BUILDDIR)/libsplexer.so
	$(CC) $(CFLAGS) -ggdb -o $@ $< -L$(BUILDDIR) -I. -l:libsplexer.so

$(BUILDDIR)/lib%.so: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	gcc -shared $< -o $@

$(BUIlDDIR)/lib%.a: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	ar rcs $@ $<

$(OBJDIR)/%.o: %.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -ggdb -fPIC -o $@ -c $<

valgrind: main
	LD_LIBRARY_PATH=./build valgrind --leak-check=full --track-origins=yes ./main

clean:
	rm -rf *.o
