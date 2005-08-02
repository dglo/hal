.SUFFIXES: .c .o .S .elf .bin

KOBJS = ../lib/crt0.o ../lib/libkernel.a

all: ../bin/real-app.bin.gz

.c.o:
	$(CC) -c $(CFLAGS) $<

.S.o:
	$(CPP) $(CPPFLAGS) -o $*.i $<
	$(AS) $(AFLAGS) -o $*.o $*.i

.elf.bin:
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(CPP) $(CPPFLAGS) -DBINFILE=\"$*.bin\" -o $*-raw.i $(RAWS)
	$(AS) $(AFLAGS) -o $*-raw.o $*-raw.i
	$(LD) --script=$(RAWX) -o $*-raw.elf $*-raw.o
	$(OBJCOPY) -O binary $*-raw.elf $*.bin

../bin/%.bin.gz: %.bin; gzip -c $< > $@

../bin/real-app.bin.gz: real-app.bin

../bin/real-app: real-app.o
	gcc -Wall -I../Linux-i386/public -L../Linux-i386/lib \
		-DICESOFT_BUILD=\"$(ICESOFT_BUILD)\" \
		-o real-app real-app.c -lhal

clean:
	rm -f real-app *.o

real-app.elf: real-app.o $(KERNELX) $(LIBHAL) $(SYSLIBS) $(KOBJS)
	$(LD) --script=$(KERNELX) -o real-app.elf \
		$(KOBJS) real-app.o \
			$(LIBHAL) $(SYSLIBS)

