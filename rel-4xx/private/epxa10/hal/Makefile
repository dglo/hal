#
# libhal makefile, used in conjuction with the workspace
# makefile
#
OBJS = hal.o fpga-hal.o fb-hal.o comm-hal.o domapp-hal.o

%.o : %.c *.h
	$(CC) -c $(CFLAGS) $<

all: $(LIBHAL)

clean:
	rm -f $(OBJS)

$(LIBHAL): $(OBJS)
	$(AR) r $(LIBHAL) $(OBJS)
