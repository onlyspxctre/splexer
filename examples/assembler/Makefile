.phony: all clean

all: assembler

assembler: assembler.c $(BUILDDIR)/libsplexer.so
	$(CC) $(CFLAGS) -ggdb -o $@ $< -L$(BUILDDIR) -l:libsplexer.so

clean:
	rm -rf assembler
