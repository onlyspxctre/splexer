export BUILDDIR := $(abspath ./build)
export OBJDIR := $(abspath ./obj)
export INCLUDEDIR := $(abspath ./include)

EXAMPLEDIR := ./examples
EXAMPLES := assembler

export CC := clang
export CFLAGS := -Wall -Wextra -std=c11 -fcolor-diagnostics -I$(INCLUDEDIR)

.PHONY: all clean $(EXAMPLES)

all: main $(EXAMPLES)

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

$(EXAMPLES):
	$(MAKE) -C $(EXAMPLEDIR)/$@

clean:
	rm -rf *.o
	rm -rf main
	rm -rf $(INCLUDEDIR)
