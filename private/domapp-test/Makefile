.SUFFIXES: .c .o .S .elf .bin

KOBJS = ../lib/crt0.o ../lib/libkernel.a

all: ../bin/domapp-test.bin.gz ../bin/deadtime-test.bin.gz

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

../bin/domapp-test.bin.gz: domapp-test.bin

../bin/domapp-test: real-app.o base64.o
	gcc -Wall -I../Linux-i386/public -L../Linux-i386/lib \
		-DICESOFT_BUILD=\"$(ICESOFT_BUILD)\" \
		-o domapp-test real-app.o base64.o -lhal

../bin/decode64: decode64.o base64.o
	gcc -Wall -I../Linux-i386/public -L../Linux-i386/lib \
		-o ../bin/decode64 decode64.o base64.o

../bin/encode64: encode64.o base64.o
	gcc -Wall -I../Linux-i386/public -L../Linux-i386/lib \
		-o ../bin/encode64 encode64.o base64.o

clean:
	rm -f real-app domapp-test *.o *.elf domapp-test-raw* domapp-test.bin

domapp-test.elf: real-app.o base64.o $(KERNELX) $(LIBHAL) $(SYSLIBS) $(KOBJS)
	$(LD) --script=$(KERNELX) -o domapp-test.elf \
		$(KOBJS) real-app.o base64.o \
			$(LIBHAL) $(SYSLIBS)

deadtime-test.elf: deadtime-test.o $(KERNELX) $(LIBHAL) $(SYSLIBS) $(KOBJS)
	$(LD) --script=$(KERNELX) -o deadtime-test.elf \
		$(KOBJS) deadtime-test.o $(LIBHAL) $(SYSLIBS)


