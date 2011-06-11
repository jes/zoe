# Makefile for zoe
# James Stanley 2011
#
# Run with "make cflags=foo" to add to CFLAGS (same for LDFLAGS)

LDFLAGS = $(ldflags)
CFLAGS  = -DASM_BITSCAN $(cflags)
OBJS    = bitscan.o board.o game.o move.o search.o zoe.o

.PHONY: all
all: zoe

.PHONY: clean
clean:
	rm -f $(OBJS)

zoe: $(OBJS)
	$(CC) -o zoe $(LDFLAGS) $(OBJS)

%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $<
