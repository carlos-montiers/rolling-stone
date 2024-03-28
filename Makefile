#CC = purify -freeze-on-error=yes -inuse-at-exit=yes gcc
CC = gcc

# Ensure 32-bit architecture compilation and disable compiler optimizations
OPT = -m32 -O0

#WARN = -Wall

PC = -DPC

# Set stack size to 8 MB for Windows and Linux compatibility
ifeq ($(OS),Windows_NT)
  LDFLAGS = -Wl,--stack,8388608
else
  LDFLAGS = -Wl,-z,stack-size=8388608
endif

#DEBUG = -DDEBUG
#GTV = -DGTV

OPTIONS = $(DEBUG) $(GTV) $(PC)
CFLAGS = $(OPT) $(WARN) $(INCLUDE) $(LIBDIRS) $(OPTIONS)

RS:

OBJ = moves.o debug.o ida.o deadlock.o bitstring.o pensearch.o\
 init.o io.o mark.o conflicts.o deadsearch.o dl.o mymem.o histogram.o\
 weights.o lowerbound.o hashtable.o stats.o tree.o menu.o macro.o gtv.o time.o

board.o: board.h  init.h

init.o: board.h

RS: $(OBJ) board.h  init.h dl.c
	$(CC) $(CFLAGS) -o RS $(OBJ) $(LIB) $(LDFLAGS)

RStest: $(OBJ) board.h  init.h dl.c
	$(CC) $(CFLAGS) -o RStest $(OBJ) $(LIB)

All: clean RS

clean:
	rm -f $(OBJ) RS
