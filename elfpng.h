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
#ifndef ELFPNG_HEADER
#define ELFPNG_HEADER

#include <byteswap.h>
#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


#define ELFPNG_SECPFX      ".png."
#define ELFPNG_SECPFX_LEN  5


struct elfpng_section {
    uint8_t  *data;  /* pointer to PNG data */
    size_t    size;  /* PNG data size */
    uint32_t  w;     /* icon width */
    uint32_t  h;     /* icon height */
};


/**
 * Open a file and create a file mapping
 *
 * On success a pointer to a mapped area is returned and the file's size
 * is saved to the `filesize' parameter.
 * The mapping must later be deleted with a call to `munmap()'.
 *
 * On error MAP_FAILED is returned and the value of `filesize' is undefined.
 */
void *elfpng_open_file(const char *file, size_t *filesize);


/**
 * Receive a list of `struct elfpng_section' data. The number of entries is
 * saved to the `num' parameter.
 * The returned pointer must later be released with `free()'.
 *
 * On error NULL is returned and the value of `num' is undefined.
 */
struct elfpng_section *elfpng_data(void *map_addr, size_t filesize, size_t *num);



/**********************************************************************************/

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define ELFDATA2NATIVE   ELFDATA2LSB
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define ELFDATA2NATIVE   ELFDATA2MSB
#else
# error "unknown byte order"
#endif


static uint16_t elfpng_val_16(uint16_t x, uint8_t order) {
    return (order == ELFDATA2NATIVE) ? x : bswap_16(x);
}

static uint32_t elfpng_val_32(uint32_t x, uint8_t order) {
    return (order == ELFDATA2NATIVE) ? x : bswap_32(x);
}

static uint64_t elfpng_val_64(uint64_t x, uint8_t order) {
    return (order == ELFDATA2NATIVE) ? x : bswap_64(x);
}

static uint32_t elfpng_ntoh32(uint8_t *data) {
    uint32_t *val = (uint32_t *)data;
    return (ELFDATA2NATIVE == ELFDATA2MSB) ? *val : bswap_32(*val);
}


#define TEMPLATE_ELFPNG_DATA(PSEC, UINTXX_T, ELF_EHDR, ELF_SHDR, ELFPNG_VAL_XX) \
{ \
    ELF_EHDR *ehdr; \
    ELF_SHDR *shdr; \
    UINTXX_T shoff, shnum, shstrndx, strtab_off; \
    UINTXX_T i; \
\
    const char hdr[] = \
        "\x89PNG\r\n\x1A\n" /* PNG magic bytes */ \
        "\x00\x00\x00\x0D"  /* chunk data length (13 bytes), big endian */ \
        "IHDR"              /* chunk type */ \
        /* ... */;          /* + width + height */ \
\
    const size_t hdrlen = sizeof(hdr) - 1; \
\
    ehdr = (ELF_EHDR *)addr; \
    shoff = ELFPNG_VAL_XX(ehdr->e_shoff, order); \
    shnum = elfpng_val_16(ehdr->e_shnum, order); \
    shstrndx = elfpng_val_16(ehdr->e_shstrndx, order); \
\
    if (shoff >= filesize || shoff == 0) { \
        return NULL; \
    } \
\
    shdr = (ELF_SHDR *)(addr + shoff); \
\
    /* special index number values */ \
    if ((shnum == 0 && (shnum = ELFPNG_VAL_XX(shdr[0].sh_size, order)) == 0) || \
        (shstrndx == SHN_XINDEX && (shstrndx = ELFPNG_VAL_XX(shdr[0].sh_link, order)) == 0)) \
    { \
        return NULL; \
    } \
\
    /* strtab offset */ \
    strtab_off = ELFPNG_VAL_XX(shdr[shstrndx].sh_offset, order); \
\
    if (strtab_off >= filesize) { \
        return NULL; \
    } \
\
    /* look for icon sections */ \
    for (i = 1; i < shnum; ++i) { \
        UINTXX_T sh_name, sh_offset, sh_size; \
        const char *name; \
        uint8_t *data; \
\
        if (i == shstrndx) { \
            continue; \
        } \
\
        sh_name = ELFPNG_VAL_XX(shdr[i].sh_name, order); \
        sh_offset = ELFPNG_VAL_XX(shdr[i].sh_offset, order); \
        sh_size = ELFPNG_VAL_XX(shdr[i].sh_size, order); \
\
        if (sh_offset >= filesize || (strtab_off + sh_name) >= filesize || \
            sh_size < hdrlen + 8) \
        { \
            continue; \
        } \
\
        /* section name */ \
        name = (const char *)(addr + strtab_off + sh_name); \
\
        if (strncmp(name, ELFPNG_SECPFX, ELFPNG_SECPFX_LEN) != 0 || \
            name[ELFPNG_SECPFX_LEN] == 0) \
        { \
            continue; \
        } \
\
        /* section data */ \
        data = addr + sh_offset; \
\
        if (memcmp(data, hdr, hdrlen) == 0) { \
            /* append PNG section info */ \
            PSEC = (struct elfpng_section *)realloc(PSEC, sizeof(struct elfpng_section) * (*num + 1)); \
            PSEC[*num].data = data; \
            PSEC[*num].size = sh_size; \
            PSEC[*num].w = elfpng_ntoh32(data + hdrlen); \
            PSEC[*num].h = elfpng_ntoh32(data + hdrlen + 4); \
            *num = *num + 1; \
        } \
    } \
}

static struct elfpng_section *elfpng_data32(uint8_t *addr, size_t filesize, size_t *num, uint8_t order)
{
    struct elfpng_section *sec = NULL;
    TEMPLATE_ELFPNG_DATA(sec, uint32_t, Elf32_Ehdr, Elf32_Shdr, elfpng_val_32);

    return sec;
}

static struct elfpng_section *elfpng_data64(uint8_t *addr, size_t filesize, size_t *num, uint8_t order)
{
    struct elfpng_section *sec = NULL;
    TEMPLATE_ELFPNG_DATA(sec, uint64_t, Elf64_Ehdr, Elf64_Shdr, elfpng_val_64);

    return sec;
}

#undef TEMPLATE_ELFPNG_DATA


void *elfpng_open_file(const char *file, size_t *filesize)
{
    int fd;
    struct stat st;
    uint8_t buf[EI_NIDENT];
    void *addr = MAP_FAILED;

    if (filesize) {
        *filesize = 0;
    }

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

    /* check ELF class and byte order */
    if (!(buf[EI_CLASS] == ELFCLASS64 || buf[EI_CLASS] == ELFCLASS32) ||
        !(buf[EI_DATA] == ELFDATA2LSB || buf[EI_DATA] == ELFDATA2MSB))
    {
        close(fd);
        return MAP_FAILED;
    }

    /* filesize */
    if (fstat(fd, &st) != -1 && st.st_size > (off_t)(sizeof(Elf64_Ehdr))) {
        addr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        *filesize = st.st_size;
    }

    close(fd);

    return addr;
}


struct elfpng_section *elfpng_data(void *map_addr, size_t filesize, size_t *num)
{
    uint8_t *addr, order;

    if (num) {
        *num = 0;
    }

    if (map_addr == MAP_FAILED || filesize < sizeof(Elf64_Ehdr) || !num) {
        return NULL;
    }

    addr = (uint8_t *)map_addr;
    order = addr[EI_DATA];

    /* check byte order */
    if (order != ELFDATA2LSB && order != ELFDATA2MSB) {
        return NULL;
    }

    /* get section header data */
    if (addr[EI_CLASS] == ELFCLASS64) {
        return elfpng_data64(addr, filesize, num, order);
    } else if (addr[EI_CLASS] == ELFCLASS32) {
        return elfpng_data32(addr, filesize, num, order);
    }

    return NULL;
}

#endif /* !ELFPNG_HEADER */

