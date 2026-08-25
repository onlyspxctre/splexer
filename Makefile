export BUILDDIR := $(abspath ./build)
export OBJDIR := $(abspath ./obj)
export INCLUDEDIR := $(abspath ./include)

export CC := clang
export CFLAGS := -Wall -Wextra -std=c11 -fcolor-diagnostics -I$(INCLUDEDIR)
export LDFLAGS := -fuse-ld=lld

EXAMPLEDIR := ./examples
EXAMPLES := assembler

.PHONY: all clean $(EXAMPLES)

ifneq ($(GRANULAR_TOK_UNKNOWN),)
override EXTRAFLAGS += -DGRANULAR_TOK_UNKNOWN
endif

ifneq ($(NO_MULTICOMMENT),)
override EXTRAFLAGS += -DNO_MULTICOMMENT
endif

ifneq ($(RELEASE),)
CFLAGS += -O2 -flto
else
CFLAGS += -g
endif

ifneq ($(WINDOWS),)
CC +=  --target=x86_64-w64-mingw32 --sysroot=/usr/x86_64-w64-mingw32
LDFLAGS += -L/usr/lib/gcc/x86_64-w64-mingw32/16.1.0

all: $(BUILDDIR)/splexer.dll $(BUILDDIR)/libsplexer-win.a

main.exe: main.c $(BUILDDIR)/splexer.dll
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(BUILDDIR) -I. -l:splexer.dll

$(BUILDDIR)/%.dll: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	$(CC) $(LDFLAGS) -shared $< -o $@ -Wl,--out-implib,$(BUILDDIR)/lib$*.dll.a -Wl,--output-def,$(BUILDDIR)/$*.def

$(BUILDDIR)/lib%-win.a: $(OBJDIR)/%-static.o
	mkdir -p $(BUILDDIR)
	ar rcs $@ $<

$(OBJDIR)/%.o: %.c $(INCLUDEDIR)/sptl.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -DSP_WIN32_EXPORT $(EXTRAFLAGS) -o $@ -c $<

else
all: $(BUILDDIR)/libsplexer.so $(BUILDDIR)/libsplexer.a

main: main.c $(BUILDDIR)/libsplexer.so
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(BUILDDIR) -I. -l:libsplexer.so

$(BUILDDIR)/lib%.so: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	$(CC) $(LDFLAGS) -shared $< -o $@

$(BUILDDIR)/lib%.a: $(OBJDIR)/%-static.o
	mkdir -p $(BUILDDIR)
	ar rcs $@ $<

$(OBJDIR)/%.o: %.c $(INCLUDEDIR)/sptl.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -fPIC $(EXTRAFLAGS) -o $@ -c $<

endif

$(OBJDIR)/%-static.o: %.c $(INCLUDEDIR)/sptl.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -DSP_STATIC $(EXTRAFLAGS) -o $@ -c $<

$(INCLUDEDIR)/sptl.h:
	mkdir -p $(INCLUDEDIR)
	cd $(INCLUDEDIR) && curl -O https://raw.githubusercontent.com/onlyspxctre/sptl.h/refs/heads/master/sptl.h

valgrind: main
	LD_LIBRARY_PATH=./build valgrind --leak-check=full --track-origins=yes ./main

$(EXAMPLES):
	$(MAKE) -C $(EXAMPLEDIR)/$@

clean:
	rm -rf *.o
	rm -rf *.exe
	rm -rf main
	rm -rf $(BUILDDIR)
	rm -rf $(OBJDIR)
	rm -rf $(INCLUDEDIR)
