OBJS = emu.o
CC = gcc
CFLAGS = -Wall -I../include

.c.o:
	$(CC) -c $(CFLAGS) $<

all: ../lib/libdomhal.a

clean:
	rm -f ../lib/libdomhal.a $(OBJS)

../lib/libdomhal.a: $(OBJS)
	$(AR) r ../lib/libdomhal.a $(OBJS)