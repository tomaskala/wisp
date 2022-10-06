.POSIX:
.SUFFIXES:
CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Werror
LDFLAGS =
LDLIBS  =

SRCS = \
	src/scanner.c

DBGEXE     = dbg
DBGOBJS    = src/main.dbg.o $(SRCS:.c=.dbg.o)
DBGCFLAGS  = -Og -g -fsanitize=address -fsanitize=leak -fsanitize=undefined
DBGLDFLAGS = -fsanitize=address -fsanitize=leak -fsanitize=undefined

RELEXE     = lisp
RELOBJS    = src/main.o $(SRCS:.c=.o)
RELCFLAGS  = -O3
RELLDFLAGS =

TSTEXE     = tests
TSTOBJS    = test/tests.tst.o $(SRCS:.c=.tst.o)

.PHONY: all
all: $(RELEXE)

.PHONY: debug
debug: $(DBGEXE)

.PHONY: check
check: $(TSTEXE)
	./$(TSTEXE)

$(RELEXE): $(RELOBJS)
	$(CC) $(LDFLAGS) $(RELLDFLAGS) -o $(RELEXE) $(RELOBJS) $(LDLIBS)

$(DBGEXE): $(DBGOBJS)
	$(CC) $(LDFLAGS) $(DBGLDFLAGS) -o $(DBGEXE) $(DBGOBJS) $(LDLIBS)

$(TSTEXE): $(TSTOBJS)
	$(CC) $(LDFLAGS) $(DBGLDFLAGS) -o $(TSTEXE) $(TSTOBJS) $(LDLIBS)

.PHONY: clean
clean:
	rm -f $(RELEXE) $(RELOBJS) $(DBGEXE) $(DBGOBJS) $(TSTEXE) $(TSTOBJS)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(RELCFLAGS) -o $@ -c $<

.SUFFIXES: .c .dbg.o
.c.dbg.o:
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $@ -c $<

.SUFFIXES: .c .tst.o
.c.tst.o:
	$(CC) $(CFLAGS) $(DBGCFLAGS) -o $@ -c $<
