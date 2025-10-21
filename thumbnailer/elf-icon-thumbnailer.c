/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "../elfpng.h"


static int save_icon(const char *fin, const char *fout, uint32_t h)
{
    size_t i, n, num, filesize, remain, nbytes;
    uint8_t *ptr;
    void *addr = MAP_FAILED;
    struct elfpng_section *sec = NULL;
    FILE *fpout = NULL;
    int rv = 1;

    /* load section data */
    if ((addr = elfpng_open_file(fin, &filesize)) == MAP_FAILED) {
        return 1;
    }

    if ((sec = elfpng_data(addr, filesize, &num)) == NULL) {
        goto JMP_END;
    }

    /* open output file for writing */
    if ((fpout = fopen(fout, "wb")) == NULL) {
        goto JMP_END;
    }

    /* find icon with desired height or largest icon */
    for (i = n = 0; i < num; i++) {
        if (sec[i].h == h) {
            n = i;
            break;
        } else if (sec[i].h > sec[n].h) {
            n = i;
        }
    }

    /* save data */
    nbytes = 512*1024;
    ptr = sec[n].data;
    remain = sec[n].size;

    for ( ; remain != 0; ptr += nbytes, remain -= nbytes) {
        if (remain < nbytes) {
            nbytes = remain;
        }

        if (fwrite(ptr, 1, nbytes, fpout) != nbytes) {
            if (ferror(fpout) || !feof(fpout)) {
                unlink(fout);
                goto JMP_END;
            }

            break;
        }
    }

    rv = 0;

JMP_END:

    if (addr != MAP_FAILED) {
        munmap(addr, filesize);
    }

    if (fpout) {
        fclose(fpout);
    }

    free(sec);

    return rv;
}


static uint32_t str_to_uint32(const char *str)
{
    unsigned long l;
    char *endptr;

    errno = 0;
    l = strtoul(str, &endptr, 10);

    if (errno != 0 || *endptr != 0 || l >= UINT32_MAX) {
        return UINT32_MAX;
    }

    return (uint32_t)l;
}


/**
 * Thumbnailer Entry line must be `Exec=elf-icon-thumbnailer %s %i %o'
 * %i = input file path
 * %u = input file URI
 * %o = output file path
 * %s = vertical thumbnail size
 */
int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <size> <input> <output>\n", argv[0]);
        fprintf(stderr, "\n%s\n",
            "[Thumbnailer Entry]\n"
            "TryExec=elf-icon-thumbnailer\n"
            "Exec=elf-icon-thumbnailer %s %i %o\n"
            "MimeType=application/x-executable;application/x-pie-executable;\n");

        return 1;
    }

    uint32_t size = str_to_uint32(argv[1]);

    if (size == UINT32_MAX) {
        return 1;
    }

    return save_icon(argv[2], argv[3], size);
}
