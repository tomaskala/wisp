.POSIX:
.SUFFIXES:
CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Werror
LDFLAGS =
LDLIBS  =

SRCS = \
	src/main.c \
	src/scanner.c

DBGEXE     = dbg
DBGOBJS    = $(SRCS:.c=.dbg.o)
DBGCFLAGS  = -Og -g -fsanitize=address -fsanitize=leak -fsanitize=undefined
DBGLDFLAGS = -fsanitize=address -fsanitize=leak -fsanitize=undefined

RELEXE     = lisp
RELOBJS    = $(SRCS:.c=.o)
RELCFLAGS  = -O3
RELLDFLAGS =

.PHONY: all
all: $(RELEXE)

.PHONY: debug
debug: $(DBGEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(LDFLAGS) $(RELLDFLAGS) -o $(RELEXE) $(RELOBJS) $(LDLIBS)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(LDFLAGS) $(DBGLDFLAGS) -o $(DBGEXE) $(DBGOBJS) $(LDLIBS)

.PHONY: clean
clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $@ -c $<

.SUFFIXES: .c .dbg.o
.c.dbg.o:
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $@ -c $<
