all: ../$(PLATFORM)/bin/real-app.bin.gz

../epxa10/bin/real-app.bin.gz: real-app.elf

../Linux-i386/bin/real-app: real-app.o
	gcc -Wall -I../Linux-i386/public -L../Linux-i386/lib \
		-DICESOFT_BUILD=\"$(ICESOFT_BUILD)\" \
		-o real-app real-app.c -lhal

clean:
	rm -f real-app *.o

real-app.elf: real-app.o $(KERNELX) $(LIBHAL)
	$(LD) --script=$(KERNELX) -o stfserv.elf \
		$(KOBJS) stfserv.o $(STFOBJS) $(EXOBJS) \
			$(LIBHAL) $(LIBEXPAT) $(SYSLIBS)

