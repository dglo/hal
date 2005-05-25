all: html/index.html

html/index.html: include/DOM_MB_hal.h
	doxygen config/doxygen.conf

clean:
	rm -rf html/*

install:
	cp html/* ~/public_html/dom-mb
