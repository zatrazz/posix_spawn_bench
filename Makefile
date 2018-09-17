CC=gcc
CFLAGS=-Wall -std=c11
ifdef DEBUG
CFLAGS+=-O0 -g
else
CFLAGS+=-O3
endif

TARGETS= \
  posix_spawn_bench

all:	$(TARGETS)

posix_spawn_bench:		posix_spawn_bench.o
				$(CC) $^ -o $@

dummy:				dummy.c
				$(CC) $^ -o $@

clean:
				rm -f *.o $(TARGETS) perf.data*
