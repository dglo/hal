EMUOBJS = emu.o
OBJS = hal.o
CC = gcc
CFLAGS = -Wall -I../include
LIBEMU = ../lib/libdomhalemu.a
LIBHAL = ../lib/libdomhal.a

.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(LIBEMU) $(LIBHAL)

clean:
	rm -f $(LIBEMU) $(LIBHAL) $(OBJS) $(EMUOBJS)

$(LIBEMU): $(EMUOBJS)
	$(AR) r $(LIBEMU) $(EMUOBJS)

$(LIBHAL): $(OBJS)
	$(AR) r $(LIBHAL) $(OBJS)