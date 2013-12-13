#
# libhal makefile, used in conjuction with the workspace
# makefile
#
OBJS = hal.o

%.o : %.c
	$(CC) -m32 -c $(CFLAGS) $<

all: $(LIBHAL)

clean:
	rm -f $(OBJS)

$(LIBHAL): $(OBJS)
	$(AR) r $(LIBHAL) $(OBJS)
