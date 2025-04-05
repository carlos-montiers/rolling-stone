#CC = purify -freeze-on-error=yes -inuse-at-exit=yes gcc
CC = gcc

OPT = -O2 -s -flto
ifeq ($(OS),Windows_NT)
  OPT += -D_WIN32
endif

#WARN = -Wall

PC = -DPC

# Set stack size for platform compatibility
STACK_SIZE = 8388608 # 8 MB
ifeq ($(OS),Windows_NT)
  LDFLAGS = -Wl,-strip-all,--stack,$(STACK_SIZE)
else
  LDFLAGS = -Wl,-strip-all,-z,stack-size=$(STACK_SIZE)
endif

#DEBUG = -DDEBUG
#GTV = -DGTV

OPTIONS = $(DEBUG) $(GTV) $(PC)
CFLAGS = $(OPT) $(WARN) $(INCLUDE) $(LIBDIRS) $(OPTIONS)

RS:

ifeq ($(OS),Windows_NT)
version.o: version.rc
	windres version.rc -o version.o
endif

OBJ = moves.o debug.o ida.o deadlock.o bitstring.o pensearch.o\
 init.o io.o mark.o conflicts.o deadsearch.o dl.o mymem.o histogram.o\
 weights.o lowerbound.o hashtable.o stats.o tree.o menu.o macro.o gtv.o time.o
ifeq ($(OS),Windows_NT)
 OBJ += version.o
endif

board.o: board.h  init.h

init.o: board.h

RS: $(OBJ) board.h  init.h dl.c
	$(CC) $(CFLAGS) -o RS $(OBJ) $(LIB) $(LDFLAGS)

RStest: $(OBJ) board.h  init.h dl.c
	$(CC) $(CFLAGS) -o RStest $(OBJ) $(LIB)

All: clean RS

clean:
	rm -f $(OBJ) RS
