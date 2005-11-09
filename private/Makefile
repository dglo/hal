all: html/index.html
	cd libdomhal; make all

html/index.html: include/DOM_MB_hal.h
	doxygen config/doxygen.conf

clean:
	rm -rf html/*
	cd libdomhal; make clean

install:
	cp html/*.css html/*.html html/*.png ~/public_html/dom-mb
