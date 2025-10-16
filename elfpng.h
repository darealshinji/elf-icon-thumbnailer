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


#define ELFPNG_SPFX      ".png."
#define ELFPNG_SPFX_LEN  5


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
struct elfpng_section *elfpng_data(void *map_addr, size_t filesize, size_t *num);



/**********************************************************************************/

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define ELFDATA2NATIVE   ELFDATA2LSB
# define ELFPNG_NTOH32(x) (bswap_32(x))
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define ELFDATA2NATIVE   ELFDATA2MSB
# define ELFPNG_NTOH32(x) (x)
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


static struct elfpng_section *elfpng_save_png_section(struct elfpng_section *sec, size_t *num, uint8_t *data, size_t datasize)
{
    struct _elfpng_header {
        uint8_t   hdr[16];  /* PNG magic bytes + begin of first data chunk */
        uint32_t  width;    /* image width, big endian */
        uint32_t  height;   /* image height, big endian */
    };

    const char * const header_chunk =
        "\x89PNG\r\n\x1A\n" /* PNG magic bytes */
        "\x00\x00\x00\x0D"  /* chunk data length, big endian */
        "IHDR";             /* chunk type */

    /* check PNG magic bytes plus begin of IHDR chunk */
    if (datasize >= sizeof(struct _elfpng_header)) {
        struct _elfpng_header *hdr = (struct _elfpng_header *)data;

        if (memcmp(header_chunk, hdr->hdr, sizeof(hdr->hdr)) == 0) {
            sec = (struct elfpng_section *)realloc(sec, sizeof(struct elfpng_section) * (*num + 1));

            sec[*num].data = data;
            sec[*num].datasize = datasize;

            /* these values are always in network byte order */
            sec[*num].width = ELFPNG_NTOH32(hdr->width);
            sec[*num].height = ELFPNG_NTOH32(hdr->height);

            *num = *num + 1;
        }
    }

    return sec;
}


static struct elfpng_section *elfpng_data32(uint8_t *addr, size_t filesize, size_t *num, uint8_t order)
{
    struct elfpng_section *sec = NULL;

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)addr;
    uint32_t shoff = elfpng_val_32(ehdr->e_shoff, order);
    uint32_t shnum = elfpng_val_16(ehdr->e_shnum, order);
    uint32_t shstrndx = elfpng_val_16(ehdr->e_shstrndx, order);

    if (shoff >= filesize || shoff == 0) {
        return NULL;
    }

    Elf32_Shdr *shdr = (Elf32_Shdr *)(addr + shoff);

    /* special index number values */
    if ((shnum == 0 && (shnum = elfpng_val_32(shdr[0].sh_size, order)) == 0) ||
        (shstrndx == SHN_XINDEX && (shstrndx = elfpng_val_32(shdr[0].sh_link, order)) == 0))
    {
        return NULL;
    }

    /* strtab offset */
    uint32_t strtab_off = elfpng_val_32(shdr[shstrndx].sh_offset, order);

    if (strtab_off >= filesize) {
        return NULL;
    }

    /* look for icon sections */
    for (uint32_t i = 0; i < shnum; i++) {
        if (i == shstrndx) {
            continue;
        }

        uint32_t sh_name = elfpng_val_32(shdr[i].sh_name, order);
        uint32_t sh_offset = elfpng_val_32(shdr[i].sh_offset, order);
        uint32_t sh_size = elfpng_val_32(shdr[i].sh_size, order);

        if (sh_offset >= filesize || (strtab_off + sh_name) >= filesize) {
            continue;
        }

        const char *name = (const char *)(addr + strtab_off + sh_name);

        if (strncmp(name, ELFPNG_SPFX, ELFPNG_SPFX_LEN) == 0 &&
            name[ELFPNG_SPFX_LEN] != 0)
        {
            /* append PNG section info */
            sec = elfpng_save_png_section(sec, num, addr + sh_offset, sh_size);
        }
    }

    return sec;
}


static struct elfpng_section *elfpng_data64(uint8_t *addr, size_t filesize, size_t *num, uint8_t order)
{
    struct elfpng_section *sec = NULL;

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)addr;
    uint64_t shoff = elfpng_val_64(ehdr->e_shoff, order);
    uint64_t shnum = elfpng_val_16(ehdr->e_shnum, order);
    uint64_t shstrndx = elfpng_val_16(ehdr->e_shstrndx, order);

    if (shoff >= filesize || shoff == 0) {
        return NULL;
    }

    Elf64_Shdr *shdr = (Elf64_Shdr *)(addr + shoff);

    /* special index number values */
    if ((shnum == 0 && (shnum = elfpng_val_64(shdr[0].sh_size, order)) == 0) ||
        (shstrndx == SHN_XINDEX && (shstrndx = elfpng_val_64(shdr[0].sh_link, order)) == 0))
    {
        return NULL;
    }

    /* strtab offset */
    uint64_t strtab_off = elfpng_val_64(shdr[shstrndx].sh_offset, order);

    if (strtab_off >= filesize) {
        return NULL;
    }

    /* look for icon sections */
    for (uint64_t i = 0; i < shnum; i++) {
        if (i == shstrndx) {
            continue;
        }

        uint64_t sh_name = elfpng_val_64(shdr[i].sh_name, order);
        uint64_t sh_offset = elfpng_val_64(shdr[i].sh_offset, order);
        uint64_t sh_size = elfpng_val_64(shdr[i].sh_size, order);

        if (sh_offset >= filesize || (strtab_off + sh_name) >= filesize) {
            continue;
        }

        const char *name = (const char *)(addr + strtab_off + sh_name);

        if (strncmp(name, ELFPNG_SPFX, ELFPNG_SPFX_LEN) == 0 &&
            name[ELFPNG_SPFX_LEN] != 0)
        {
            /* append PNG section info */
            sec = elfpng_save_png_section(sec, num, addr + sh_offset, sh_size);
        }
    }

    return sec;
}


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
    if (map_addr == MAP_FAILED || filesize < sizeof(Elf64_Ehdr) || !num) {
        return NULL;
    }

    uint8_t *addr = (uint8_t *)map_addr;
    uint8_t order = addr[EI_DATA];

    /* byte order check */
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

