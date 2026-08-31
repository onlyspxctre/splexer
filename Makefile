export BUILDDIR := $(abspath ./build)
export OBJDIR := $(abspath ./obj)
export INCLUDEDIR := $(abspath ./include)
DEPSDIR := $(abspath ./deps)

export CC := clang
export AR := llvm-ar
export CFLAGS := -Wall -Wextra -std=c11 -fcolor-diagnostics -I$(INCLUDEDIR)
export LDFLAGS := -fuse-ld=lld

EXAMPLEDIR := ./examples
EXAMPLES := assembler

SPTL_VERSION := dc90d34
SPTL_DIR := $(DEPSDIR)/sptl.h-$(SPTL_VERSION)

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
CC +=  --target=x86_64-w64-mingw32

all: $(BUILDDIR)/splexer.dll $(BUILDDIR)/libsplexer-win.a

main.exe: main.c $(BUILDDIR)/splexer.dll
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(BUILDDIR) -I. -l:splexer.dll

$(BUILDDIR)/%.dll: $(OBJDIR)/%.o
	mkdir -p $(BUILDDIR)
	$(CC) $(LDFLAGS) -shared $< -o $@ -Wl,--out-implib,$(BUILDDIR)/lib$*.dll.a -Wl,--output-def,$(BUILDDIR)/$*.def

$(BUILDDIR)/lib%-win.a: $(OBJDIR)/%-static.o
	mkdir -p $(BUILDDIR)
	$(AR) rcs $@ $<

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
	$(AR) rcs $@ $<

$(OBJDIR)/%.o: %.c $(INCLUDEDIR)/sptl.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -fPIC $(EXTRAFLAGS) -o $@ -c $<

endif

$(OBJDIR)/%-static.o: %.c $(INCLUDEDIR)/sptl.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -DSP_STATIC $(EXTRAFLAGS) -o $@ -c $<

$(INCLUDEDIR)/sptl.h: $(SPTL_DIR).tar.gz
	mkdir -p $(INCLUDEDIR)
	cp $(SPTL_DIR)/sptl.h $@

$(SPTL_DIR).tar.gz:
	mkdir -p $(SPTL_DIR)
	curl -fsSL -o $(SPTL_DIR).tar.gz https://github.com/onlyspxctre/sptl.h/archive/$(SPTL_VERSION).tar.gz
	tar xf $(SPTL_DIR).tar.gz -C $(SPTL_DIR) --strip-components=1

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
