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

/**
 * Define `ELFPNG_IMPLEMENTATION' to add the function implementations to your
 * source, otherwise only the prototypes will be aded.
 */

#ifndef ELFPNG_HEADER
#define ELFPNG_HEADER

#include <stdint.h>

#define ELFPNG_SECTION_PREFIX  ".png."
#define ELFPNG_SECTION_PFXLEN  5


struct elfpng_section {
    uint8_t  *data;      /* pointer to PNG data */
    size_t    datasize;  /* PNG data size */
    uint32_t  width;     /* icon width */
    uint32_t  height;    /* icon height */
};


/**
 * Open a file and create a file mapping
 *
 * On success a pointer to a mapped area is returned and the file's size
 * is saved to the `filesize' parameter.
 * The mapping must later be deleted with a call to `munmap()'.
 *
 * On error MAP_FAILED is returned and the value of `filesize' is unspecified.
 */
void *elfpng_open_file(const char *file, size_t *filesize);


/**
 * Receive a list of `struct elfpng_section' data. The number of entries is
 * saved to the `num' parameter.
 * The returned pointer must later be released with `free()'.
 *
 * On error NULL is returned and the value of `num' is unspecified.
 */
struct elfpng_section *elfpng_data(void *addr, size_t filesize, size_t *num);


#endif /* !ELFPNG_HEADER */



#ifdef ELFPNG_IMPLEMENTATION

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


struct _elfpng_header {
    uint8_t   hdr[16];  /* PNG magic bytes + begin of first data chunk */
    uint32_t  width;    /* image width, big endian */
    uint32_t  height;   /* image height, big endian */
};


/* _elfpng_parse64() */
#define ELFPNG_PARSE  _elfpng_parse64
#define ELF_EHDR      Elf64_Ehdr
#define ELF_SHDR      Elf64_Shdr
#include "elfpng.i"


/* _elfpng_parse32() */
#define ELFPNG_PARSE  _elfpng_parse32
#define ELF_EHDR      Elf32_Ehdr
#define ELF_SHDR      Elf32_Shdr
#include "elfpng.i"


void *elfpng_open_file(const char *file, size_t *filesize)
{
    int fd;
    struct stat st;
    uint8_t buf[EI_NIDENT];
    void *addr = MAP_FAILED;

    /* open file */
    if (!filesize || !file || !*file ||
        (fd = open(file, O_RDONLY)) == -1)
    {
        return MAP_FAILED;
    }

    /* magic bytes */
    if (read(fd, &buf, EI_NIDENT) != EI_NIDENT ||
        memcmp(buf, ELFMAG, SELFMAG) != 0)
    {
        close(fd);
        return MAP_FAILED;
    }

    /* elfclass, byte order */
    if (!(buf[EI_CLASS] == ELFCLASS64 || buf[EI_CLASS] == ELFCLASS32) ||
        !(buf[EI_DATA] == ELFDATA2LSB || buf[EI_DATA] == ELFDATA2MSB))
    {
        close(fd);
        return MAP_FAILED;
    }

    /* filesize */
    if (fstat(fd, &st) != -1 && st.st_size > (int)(sizeof(Elf64_Ehdr))) {
        addr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        *filesize = st.st_size;
    }

    close(fd);

    return addr;
}


struct elfpng_section *elfpng_data(void *addr, size_t filesize, size_t *num)
{
    if (addr == MAP_FAILED || !num || filesize < sizeof(Elf64_Ehdr)) {
        return NULL;
    }

    uint8_t *ptr = (uint8_t *)addr;

    if (ptr[EI_CLASS] == ELFCLASS64) {
        return _elfpng_parse64(ptr, filesize, num);
    } else if (ptr[EI_CLASS] == ELFCLASS32) {
        return _elfpng_parse32(ptr, filesize, num);
    }

    *num = 0;

    return NULL;
}

#endif /* ELFPNG_IMPLEMENTATION */

