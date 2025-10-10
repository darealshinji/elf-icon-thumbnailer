#RESOLUTIONS = 16 22 24 32 36 48 64 72 96 128 192 256 512
RESOLUTIONS = 64 192 128 256 48
# 512 is too large for Gtk?

CFLAGS   := -O3 -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS  := `pkg-config --libs gtk+-3.0` -s


all: gtk

clean:
	-rm -f gtk $(OBJS)

gtk: gtk.c
	$(CC) $(CFLAGS) -o $@ gtk.c $(LDFLAGS)
	for n in $(RESOLUTIONS) ; do \
  objcopy --add-section .png.$$n=../icons/icon_$$n.png \
    --set-section-flags .png.$$n=noload,readonly $@; \
done

gtk.c: ../elfpng.h

