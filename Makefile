.PHONY: all clean

BUILDDIR := ./build
OBJDIR := ./obj
INCLUDEDIR := ./include

CC := clang
CFLAGS := -Wall -Wextra -std=c11 -fcolor-diagnostics -I$(INCLUDEDIR)

all: main

main: main.c $(BUILDDIR)/libsplexer.so
	$(CC) $(CFLAGS) -ggdb -o $@ $< -L$(BUILDDIR) -I. -l:libsplexer.so

$(BUILDDIR)/lib%.so: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	gcc -shared $< -o $@

$(BUIlDDIR)/lib%.a: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	ar rcs $@ $<

$(OBJDIR)/%.o: %.c $(INCLUDEDIR)/sptl.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -ggdb -fPIC -o $@ -c $<

$(INCLUDEDIR)/sptl.h:
	mkdir -p $(INCLUDEDIR)
	cd $(INCLUDEDIR) && curl -O https://raw.githubusercontent.com/onlyspxctre/sptl.h/refs/heads/master/sptl.h

valgrind: main
	LD_LIBRARY_PATH=./build valgrind --leak-check=full --track-origins=yes ./main

clean:
	rm -rf *.o
	rm -rf main
	rm -rf $(INCLUDEDIR)
