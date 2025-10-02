#RESOLUTIONS = 16 22 24 32 36 48 64 72 96 128 192 256 512
RESOLUTIONS = 48 64 128 192 256
QMAKE ?= qmake


all: qt

clean:
	-rm -f qt
	$(MAKE) -f Makefile clean

distclean: clean
	$(MAKE) -f Makefile distclean

qt: Makefile qt.cpp ../elfpng.h
	$(MAKE) -f Makefile qt
	for n in $(RESOLUTIONS) ; do \
  objcopy --add-section .png.$$n=../icons/icon_$$n.png \
    --set-section-flags .png.$$n=noload,readonly $@; \
done

Makefile: qt.pro
	$(QMAKE) qt.pro

../elfpng.h: ../elfpng.i

