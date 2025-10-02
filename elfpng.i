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

#include <arpa/inet.h>
#include <byteswap.h>
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __BYTE_ORDER__
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define ELFDATA2_NATIVE ELFDATA2LSB
# elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define ELFDATA2_NATIVE ELFDATA2MSB
# else
#  error "byte order not supported"
# endif
#endif


/* switch statement will be optimized away to just one case */
#define ELFPNG_TO_HOST(VAR, x) \
    do { \
        if (addr[EI_DATA] == ELFDATA2_NATIVE) { \
            VAR = x; \
        } else { \
            switch (sizeof(x)) { \
            case 1: VAR = x; break; \
            case 2: VAR = bswap_16((uint16_t)x); break; \
            case 4: VAR = bswap_32((uint32_t)x); break; \
            case 8: VAR = bswap_64((uint64_t)x); break; \
            default: fprintf(stderr, "error: %s:%d: sizeof(" #x ") > 8\n", __FILE__, __LINE__); \
                abort(); break; \
            } \
        } \
    } while (0)


static struct elfpng_section *ELFPNG_PARSE(uint8_t *addr, size_t filesize, size_t *num)
{
    ELF_EHDR *ehdr;
    ELF_SHDR *shdr, *strtab;
    size_t version, offset, shnum, shstrndx;
    size_t strtab_off, name_off, datasize;
    size_t cnt, i;
    uint8_t *data;
    const char *name;
    struct _elfpng_header *hdr;
    struct elfpng_section *sec;

    const char *header_chunk =
        "\x89PNG\r\n\x1a\n" /* PNG magic bytes */
        "\x00\x00\x00\x0d"  /* chunk data length, big endian */
        "IHDR";             /* chunk type */

    /**
     * fallback runtime check; everything but the assignment
     * of ELFDATA2_NATIVE will be optimized away
     */
#if !defined(__BYTE_ORDER__)
    uint8_t ELFDATA2_NATIVE;
    const uint32_t u32 = 0xab0000ef;
    const uint8_t *bytes = (const uint8_t *)&u32;

    if (bytes[0] == 0xef) {
        ELFDATA2_NATIVE = ELFDATA2LSB; /* Little Endian */
    } else if (bytes[0] == 0xab) {
        ELFDATA2_NATIVE = ELFDATA2MSB; /* Big Endian */
    } else {
        fprintf(stderr, "error: %s:%d: byte order not supported\n", __FILE__, __LINE__);
        abort();
    }
#endif

    /* ELF header */
    ehdr = (ELF_EHDR *)addr;

    /* check version (ignore e_ident[EI_VERSION]) */
    ELFPNG_TO_HOST(version, ehdr->e_version);

    if (version != EV_CURRENT) {
        return NULL;
    }

    /* get section header data */
    ELFPNG_TO_HOST(offset, ehdr->e_shoff);
    ELFPNG_TO_HOST(shnum, ehdr->e_shnum);
    ELFPNG_TO_HOST(shstrndx, ehdr->e_shstrndx);

    if (offset >= filesize || shstrndx >= shnum) {
        return NULL;
    }

    shdr = (ELF_SHDR *)(addr + offset);
    strtab = &shdr[shstrndx];
    ELFPNG_TO_HOST(strtab_off, strtab->sh_offset);

    if (strtab_off >= filesize) {
        return NULL;
    }

    cnt = 0;
    sec = NULL;

    /* look for icon sections */
    for (i = 0; i < shnum; i++) {
        if (i == shstrndx) {
            continue;
        }

        ELFPNG_TO_HOST(name_off, shdr[i].sh_name);
        offset = strtab_off + name_off;
        name = (const char *)(addr + offset);

        /* check section header name */
        if (offset >= filesize ||
            strncmp(name, ELFPNG_SECTION_PREFIX, ELFPNG_SECTION_PFXLEN) != 0 ||
            name[ELFPNG_SECTION_PFXLEN] == 0)
        {
            continue;
        }

        /* get offset to data */
        ELFPNG_TO_HOST(offset, shdr[i].sh_offset);

        if (offset >= filesize) {
            continue;
        }

        /* get dimensions from PNG header */
        ELFPNG_TO_HOST(datasize, shdr[i].sh_size);

        if (datasize < sizeof(struct _elfpng_header)) {
            continue;
        }

        data = addr + offset;
        hdr = (struct _elfpng_header *)data;

        /* check PNG magic bytes plus begin of IHDR chunk */
        if (memcmp(header_chunk, hdr->hdr, sizeof(hdr->hdr)) != 0) {
            continue;
        }

        sec = (struct elfpng_section *)
            realloc(sec, sizeof(struct elfpng_section) * (cnt + 1));

        sec[cnt].data = data;
        sec[cnt].datasize = datasize;
        sec[cnt].width = ntohl(hdr->width);
        sec[cnt].height = ntohl(hdr->height);

        cnt++;
    }

    *num = cnt;

    return sec;
}

#undef ELFDATA2_NATIVE
#undef ELFPNG_TO_HOST
#undef ELFPNG_PARSE
#undef ELF_EHDR
#undef ELF_SHDR
