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

#define ELFPNG_IMPLEMENTATION
#include "../elfpng.h"


class save_icon
{
private:

    const char *m_fin, *m_fout;
    void *m_addr = MAP_FAILED;
    size_t m_filesize = 0;
    size_t m_num = 0;
    FILE *m_fpout = NULL;
    struct elfpng_section *m_sec = NULL;

public:

    save_icon(const char *f_in, const char *f_out)
    : m_fin(f_in), m_fout(f_out)
    {}

    ~save_icon()
    {
        close();
    }

    int save(uint32_t size)
    {
        size_t n = 0;

        close();

        /* load section data */
        if ((m_addr = elfpng_open_file(m_fin, &m_filesize)) == MAP_FAILED ||
            (m_sec = elfpng_data(m_addr, m_filesize, &m_num)) == NULL)
        {
            return 1;
        }

        /* open output file for writing */
        if ((m_fpout = fopen(m_fout, "wb")) == NULL) {
            return 1;
        }

        /* find icon with desired height or largest icon */
        for (size_t i = 0; i < m_num; i++) {
            if (m_sec[i].height == size) {
                n = i;
                break;
            } else if (m_sec[i].height > m_sec[n].height) {
                n = i;
            }
        }

        /* save data */
        if (fwrite(m_sec[n].data, 1, m_sec[n].datasize, m_fpout) != m_sec[n].datasize) {
            unlink(m_fout);
            return 1;
        }

        return 0;
    }

    void close()
    {
        if (m_addr != MAP_FAILED) {
            munmap(m_addr, m_filesize);
        }

        if (m_fpout) {
            fclose(m_fpout);
        }

        if (m_sec) {
            free(m_sec);
        }

        m_addr = MAP_FAILED;
        m_filesize = m_num = 0;
        m_fpout = NULL;
        m_sec = NULL;
    }
};


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

    /**
     * Thumbnailer Entry line must be:
     * Exec=elf-icon-thumbnailer %s %i %o
     * %i input file path
     * %u input file URI
     * %o output file path
     * %s vertical thumbnail size
     */
    uint32_t size = str_to_uint32(argv[1]);

    if (size == UINT32_MAX) {
        return 1;
    }

    save_icon icon(argv[2], argv[3]);

    return icon.save(size);
}
