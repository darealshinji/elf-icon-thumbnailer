On Windows it's normal to embed icon files as resources into .exe files.
The icons are used as file icons in the file manager and as window icons
by the executable itself.

Such a mechanic was never part of the ELF format standard. However, appending
data into custom sections is trivial and can easily be done with the
command line tool `objcopy`.

So my idea here is to add PNG icon files of different resolutions to the ELF file.
Each icon has its own section and the resolution is part of the section name.
This is in my opinion the easiest approach. The section names would begin
with `.png.` followed by the vertical icon resolution.

To append a 64x64 pixels icon into an ELF file:
``` sh
objcopy --add-section .png.64=icon.png --set-section-flags \
  .png.64=noload,readonly myfile
```

To append several icons from a set:
``` sh
for res in 48 64 128 192 256 ; do \
  objcopy --add-section .png.$res=icons/$res/icon.png \
    --set-section-flags .png.$res=noload,readonly myfile ; \
done
```

A section can also be removed later with `objcopy` or `strip`:
``` sh
objcopy --remove-section=.png.64 myfile
```


Thumbnailer
-----------

The directory `thumbnailer` provides a program and configuration file that allows
to preview these embedded icons in the file managers Nautilus, Nemo and Caja
(and potentially other compatible ones).


Window icons
------------

The directories `gtk-window-icon` and `qt-window-icon` are examples how to find an
icon from the current executable and use it as the default window icon.
However the Gtk example only works with Gtk3 and not Gtk4 and in most cases it's
preferrable to use the toolkit's system for icon resources.


elfpng.h
--------

Writing code that parses ELF files is not easy so I'm offering a header-only library.
The file `elfpng.i` must be in the compiler's header search path too.

Define `ELFPNG_IMPLEMENTATION` before including the header to not only add the prototypes:
``` c
#define ELFPNG_IMPLEMENTATION
#include "elfpng.h"
```

Here's an example showing how to load the largest icon from the current executable
using the path `/proc/self/exe`:
``` c
size_t filesize, num, n;

/* open file */
void *addr = elfpng_open_file("/proc/self/exe", &filesize);

if (addr != MAP_FAILED) {
    struct elfpng_section *sec = elfpng_data(addr, filesize, &num);

    if (sec != NULL) {
        /* find largest icon */
        n = 0;

        for (size_t i = 1; i < num; i++) {
            if (sec[i].height > sec[n].height) {
                n = i;
            }
        }

        /* do something with the data */
        my_window_icon_setter(sec[n].data, sec[n].datasize);
    }

    /* release data */
    free(sec);
    munmap(addr, filesize);
}

```