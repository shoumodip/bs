CFLAGS := `cat compile_flags.txt`

BINDIR := bin
LIBDIR := lib
SRCDIR := src/bs
INCDIR := include/bs
OBJDIR := $(LIBDIR)/.build

SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC))

.phony: all clean

all: setup $(BINDIR)/bs $(LIBDIR)/libbs.a $(LIBDIR)/libbs.so

clean:
	rm -rf $(BINDIR) $(LIBDIR)

setup:
	@mkdir -p $(BINDIR) $(LIBDIR)/.build

$(BINDIR)/bs: src/main.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ src/main.c $(OBJ) -Wl,-rpath=./

$(LIBDIR)/libbs.a: $(OBJ)
	ar rcs $@ $(OBJ)

$(LIBDIR)/libbs.so: $(OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJ) -Wl,-rpath=./

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<
