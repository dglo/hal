#
# libhal makefile, used in conjuction with the workspace
# makefile
#
OBJS = hal.o fpga-hal.o

.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(LIBHAL)

clean:
	rm -f $(OBJS)

$(LIBHAL): $(OBJS)
	$(AR) r $(LIBHAL) $(OBJS)
