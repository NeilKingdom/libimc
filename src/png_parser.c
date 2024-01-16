/* Required for fileno() */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif 

#include <math.h>
#include <zlib.h>
#include <alloca.h>
#include <sys/stat.h> 
#include "../include/png_parser.h"

#define _IMC_FLIP_ENDIAN(data) do { \
    size_t i; \
    uint8_t *tmp = alloca(sizeof(*(data))); \
    memcpy((void*)tmp, (void*)(data), sizeof(*(data))); \
    for (i = 0; i < sizeof(*(data)); ++i) { \
        ((uint8_t*)(data))[i] = tmp[sizeof(*(data)) - i - 1]; \
    } \
} while (0)

static size_t _imc_get_file_size(FILE *fp) {
    struct stat buf;
    int fd = fileno(fp);
    fstat(fd, &buf);
    return buf.st_size;
}

static IMC_Error _imc_validate_png_hdr(png_hndl_t png) {
    int res = memcmp((void*)png->data, (void*)PNG_MAGIC, sizeof(PNG_MAGIC)); 
    if (res != 0) {
        IMC_WARN("Not a PNG file");
        return IMC_ERROR;
    }
    fseek(png->fp, sizeof(PNG_MAGIC), SEEK_SET); /* Advance fp */

    return IMC_EOK;
}

/**
 *                    +-+-+
 * Previous scanline: |c|b|
 *                    +-+-+
 * Current scanline:  |a|x|
 *                    +-+-+
 */

static uint8_t _imc_recon_none(uint8_t *prev_scanline, uint8_t *curr_scanline, size_t idx, bool is_first) {
    return curr_scanline[idx];
}

static uint8_t _imc_recon_sub(uint8_t *prev_scanline, uint8_t *curr_scanline, size_t idx, bool is_first) {
    uint32_t x = curr_scanline[idx];
    uint32_t a = (is_first) ? 0 : curr_scanline[idx - 3]; /* TODO: change 3 to bpp */
    uint8_t res = (x + a) % 256;
    curr_scanline[idx] = res;
    return res;
}

static uint8_t _imc_recon_up(uint8_t *prev_scanline, uint8_t *curr_scanline, size_t idx, bool is_first) {
    uint32_t x = curr_scanline[idx];
    uint32_t b = prev_scanline[idx];
    uint8_t res = (x + b) % 256;
    curr_scanline[idx] = res;
    return res;
}

static uint8_t _imc_recon_avg(uint8_t *prev_scanline, uint8_t *curr_scanline, size_t idx, bool is_first) {
    uint32_t x = curr_scanline[idx];
    uint32_t a = (is_first) ? 0 : curr_scanline[idx - 3];
    uint32_t b = prev_scanline[idx];
    uint8_t res = (x + (int)floorf(((float)a + (float)b) / 2.0f)) % 256;
    curr_scanline[idx] = res;
    return res;
}

static uint32_t _imc_peath_predictor(uint8_t a, uint8_t b, uint8_t c) {
    int32_t p = a + b - c;
    int32_t pa = abs(p - a);
    int32_t pb = abs(p - b);
    int32_t pc = abs(p - c);
    int32_t pr = c;

    if (pa <= pb && pa <= pc) {
        pr = a; 
    } else if (pb <= pc) {
        pr = b;
    }

    return (uint32_t)pr;
}

static uint8_t _imc_recon_paeth(uint8_t *prev_scanline, uint8_t *curr_scanline, size_t idx, bool is_first) {
    uint32_t x = curr_scanline[idx];
    uint32_t a = (is_first) ? 0 : curr_scanline[idx - 3];
    uint32_t b = prev_scanline[idx];
    uint32_t c = (is_first) ? 0 : prev_scanline[idx - 3];
    uint8_t res = (x + _imc_peath_predictor(a, b, c)) % 256;
    curr_scanline[idx] = res;
    return res;
}

static IMC_Error _imc_read_chunk(png_hndl_t png, chunk_t *chunk) {
    /* Chunk length */
    fread((void*)&chunk->length, sizeof(chunk->length), 1, png->fp);
    _IMC_FLIP_ENDIAN(&chunk->length);

    /* Chunk type */
    fread((void*)&chunk->type, sizeof(chunk->type), 1, png->fp);

    /* Chunk data */
    if (chunk->length == 0) {
        chunk->data = NULL;
    } else {
        chunk->data = malloc(chunk->length);
        if (chunk->data == NULL) {
            IMC_WARN("Failed to allocate space for chunk data");
            return IMC_EFAULT;
        }
        fread((void*)chunk->data, chunk->length, 1, png->fp);
    }

    /* Chunk CRC */
    fread((void*)&chunk->crc, sizeof(chunk->crc), 1, png->fp);
    _IMC_FLIP_ENDIAN(&chunk->crc);

    return IMC_EOK;
}

static IMC_Error _imc_destroy_chunk_data(chunk_t *chunk) {
    if (!chunk) {
        IMC_WARN("Attempted double free on chunk");
        return IMC_EFREE;
    }

    if (chunk->data) {
        free(chunk->data);
        chunk->data = NULL;
    }

    return IMC_EOK;
}

static void _imc_chunk_to_ihdr(chunk_t *chunk, ihdr_t *ihdr) {
    assert(chunk->length + sizeof(ihdr->padding) == sizeof(*ihdr)); 
    memcpy((void*)ihdr, (void*)chunk->data, sizeof(*ihdr));
    _IMC_FLIP_ENDIAN(&ihdr->width);
    _IMC_FLIP_ENDIAN(&ihdr->height);
    _IMC_FLIP_ENDIAN(&ihdr->bit_depth);
    _IMC_FLIP_ENDIAN(&ihdr->color_type);
    _IMC_FLIP_ENDIAN(&ihdr->compress_mthd);
    assert(ihdr->compress_mthd == 0); /* PNG currently supports type 0 only */
    _IMC_FLIP_ENDIAN(&ihdr->filter_mthd);
    assert(ihdr->filter_mthd == 0); /* PNG currently supports filter method 0 only */
    _IMC_FLIP_ENDIAN(&ihdr->interlace_mthd);
}

static void _imc_print_ihdr_info(ihdr_t ihdr) {
    printf("Image width: %d\n", ihdr.width);
    printf("Image height: %d\n", ihdr.height);
    printf("Bits per-pixel: %d\n", ihdr.bit_depth);
    printf("Color type: ");
    switch (ihdr.color_type) {
        case NONE: 
            printf("Greyscale\n");
            break;
        case COLOR: 
            printf("RGB\n");
            break;
        case (PALETTE | COLOR):
            printf("Palette\n");
            break;
        case ALPHA: 
            printf("Greyscale + Alpha\n");
            break;
        case (COLOR | ALPHA):
            printf("RGBA\n");
            break;
        default:
            printf("Invalid\n");
            break;
    }
    printf("Compression method: Deflate\n");
    printf("Filter method: 0\n");
    printf("Interlaced: %s\n", (ihdr.interlace_mthd == 0) ? "False" : "True");
}

static void _imc_chunk_to_plte(chunk_t *chunk, plte_t *plte) {

}

static IMC_Error _imc_chunk_to_idat(ihdr_t *ihdr, chunk_t *chunk, idat_t *idat) {
    int res;
    z_stream stream;
    uint8_t n_channels;
    size_t scanline_len, decomp_len;

    switch (ihdr->color_type) {
        case COLOR: 
            n_channels = 3;
            break;
        case ALPHA:
            IMC_WARN("ALPHA not implemented yet");
            return IMC_ERROR;
            break;
        case PALETTE:
            IMC_WARN("PALETTE not implemented yet");
            return IMC_ERROR;
            break;
        /* TODO: Add combos */
    }

    scanline_len = ((n_channels * ihdr->width * ihdr->bit_depth + 7) >> 3) + 1; /* +1 for filter type */
    decomp_len = scanline_len * ihdr->height;
    idat->length = decomp_len;

    assert((chunk->data[0] & 0x0F) == 0x08); /* Compression type 8 = deflate */

    idat->decomp_buf = malloc(idat->length);
    if (idat->decomp_buf == NULL) {
        IMC_WARN("Failed to allocate space for decomp_buf");
        return IMC_EFAULT;
    }

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in  = Z_NULL;

    res = inflateInit(&stream);
    if (res != Z_OK) {
        IMC_WARN("Failed to initialize decompression stream");
        free(idat->decomp_buf);
        idat->decomp_buf = NULL;
        return IMC_ERROR;
    }

    stream.avail_in = chunk->length;

    do {
        if (stream.avail_in == 0) break;
        stream.next_in = chunk->data;

        do {
            stream.avail_out = idat->length;
            stream.next_out  = idat->decomp_buf;

            res = inflate(&stream, Z_NO_FLUSH);
            switch (res) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                {
                    IMC_WARN("Decompression error");
                    (void)inflateEnd(&stream);
                    free(idat->decomp_buf);
                    idat->decomp_buf = NULL;
                    return res;
                }
            }
        } while (stream.avail_out == 0);
    } while (res != Z_STREAM_END);

    /* Cleanup */
    (void)inflateEnd(&stream);
    return (res == Z_STREAM_END) ? IMC_EOK : IMC_ERROR;
}

static void _imc_chunk_to_chrm(chunk_t *chunk, chrm_t *chrm) {

}

static void _imc_chunk_to_gama(chunk_t *chunk, gama_t *gama) {

}

static void _imc_chunk_to_iccp(chunk_t *chunk, iccp_t *iccp) {

}

/*
static void _imc_chunk_to_sbit(chunk_t *chunk, sbit_t *sbit) {

}

static void _imc_chunk_to_srgb(chunk_t *chunk, srgb_t *srgb) {

}

static void _imc_chunk_to_bkgd(chunk_t *chunk, bkgd_t *bkgd) {

}

static void _imc_chunk_to_hist(chunk_t *chunk, hist_t *hist) {

}

static void _imc_chunk_to_trns(chunk_t *chunk, trns_t *trns) {

}

static IMC_Error _imc_process_next_chunk(png_hndl_t png, chunk_t *chunk) {
    if (strcmp(chunk->type, IEND) == 0) {
        return IMC_EOF;
    } else if (strcmp(chunk->type, PLTE) == 0) {
        _imc_chunk_to_plte(chunk, );
    } else if (strcmp(chunk->type, IDAT) == 0) {
        _imc_chunk_to_idat(chunk, );
    } else if (strcmp(chunk->type, CHRM) == 0) {
        _imc_chunk_to_chrm(chunk, );
    } else if (strcmp(chunk->type, GAMA) == 0) {
        _imc_chunk_to_gama(chunk, );
    } else if (strcmp(chunk->type, ICCP) == 0) {
        _imc_chunk_to_iccp(chunk, );
    } else if (strcmp(chunk->type, SBIT) == 0) {
        _imc_chunk_to_sbit(chunk, );
    } else if (strcmp(chunk->type, SRGB) == 0) {
        _imc_chunk_to_srgb(chunk, );
    } else if (strcmp(chunk->type, BKGD) == 0) {
        _imc_chunk_to_bkgd(chunk, );
    } else if (strcmp(chunk->type, HIST) == 0) {
        _imc_chunk_to_hist(chunk, );
    } else if (strcmp(chunk->type, TRNS) == 0) {
        _imc_chunk_to_trns(chunk, );
    } else if (strcmp(chunk->type, PHYS) == 0) {
        _imc_chunk_to_phys(chunk, );
    } else if (strcmp(chunk->type, SPLT) == 0) {
        _imc_chunk_to_splt(chunk, );
    } else if (strcmp(chunk->type, TIME) == 0) {
        _imc_chunk_to_time(chunk, );
    } else if (strcmp(chunk->type, ITXT) == 0) {
        _imc_chunk_to_itxt(chunk, );
    } else if (strcmp(chunk->type, TEXT) == 0) {
        _imc_chunk_to_text(chunk, );
    } else if (strcmp(chunk->type, ZTXT) == 0) {
        _imc_chunk_to_ztxt(chunk, );
    } else {
        return IMC_ERROR;
    }
}
*/

static uint8_t *_imc_reconstruct_chunk_data(ihdr_t *ihdr, uint8_t *decomp_buf, size_t decomp_len) {
    size_t x, scanline_len;
    size_t decomp_off, out_off;
    uint8_t fm, n_channels;
    uint8_t *curr_scanline = NULL;
    uint8_t *prev_scanline = NULL;
    uint8_t *out_buf = NULL;
    uint8_t (*recon_func)(uint8_t *, uint8_t *, size_t, bool);

    out_buf = malloc(decomp_len);
    if (out_buf == NULL) {
        IMC_WARN("Failed to allocate memory for output buffer");
        return NULL;
    }

    switch (ihdr->color_type) {
        case COLOR: 
            n_channels = 3;
            break;
        case ALPHA:
            IMC_WARN("ALPHA not implemented yet");
            return NULL;
            break;
        case PALETTE:
            IMC_WARN("PALETTE not implemented yet");
            return NULL;
            break;
        /* TODO: Add combos */
    }

    scanline_len = (n_channels * ihdr->width * ihdr->bit_depth + 7) >> 3;
    prev_scanline = alloca(scanline_len);
    memset((void*)prev_scanline, 0, scanline_len);

    for (decomp_off = 0, out_off = 0; decomp_off < decomp_len; decomp_off += scanline_len) {
        /* Filter method */
        fm = decomp_buf[decomp_off++];
        assert(fm <= 4);

        switch (fm) {
            case NONE: 
                recon_func = _imc_recon_none;
                break;                
            case SUB:
                recon_func = _imc_recon_sub;
                break;
            case UP:
                recon_func = _imc_recon_up;
                break;
            case AVG: 
                recon_func = _imc_recon_avg;
                break;
            case PAETH: 
                recon_func = _imc_recon_paeth;
                break;
        }

        curr_scanline = decomp_buf + decomp_off;

        for (x = 0; x < scanline_len; ++x) {
            bool is_first = (x < n_channels);
            out_buf[out_off++] = recon_func(prev_scanline, curr_scanline, x, is_first);
        }

        prev_scanline = curr_scanline;
    }

    return out_buf;
}

static void _output_raster(uint8_t *pixbuf, size_t width, size_t height) {
    size_t x, y, p;
    FILE *fp = fopen("output.ppm", "wb"); 
    fputs("P6\n", fp);
    fprintf(fp, "%ld %ld\n", width, height);
    fputs("255\n", fp);

    p = sizeof(rgb_t);
     
    for (y = 0; y < height; ++y) {
        for (x = 0; x < (width * p); x += p) {
            rgb_t pixel = {
                pixbuf[(y * p * width) + x],
                pixbuf[(y * p * width) + x + 1],
                pixbuf[(y * p * width) + x + 2]
            };

            fputc(pixel.r, fp);
            fputc(pixel.g, fp);
            fputc(pixel.b, fp);
        }
    }

    fclose(fp);
}

png_hndl_t imc_open_png(const char *path) {
    png_hndl_t png = NULL;
    FILE *fp = NULL;

    png = malloc(sizeof(png_hndl_t));
    if (!png) {
        IMC_WARN("Failed to allocate memory for png");
        return NULL;
    }

    fp = fopen(path, "r+");
    if (!fp) {
        IMC_WARN("Failed to open file");
        return NULL;
    }

    png->fp = fp;
    png->size = _imc_get_file_size(fp);
    png->data = malloc(png->size);
    if (!png->data) {
        IMC_WARN("Failed to allocate memory for png data");
        return NULL;
    }

    fread(png->data, png->size, 1, png->fp);
    if (_imc_validate_png_hdr(png) != IMC_EOK) {
        return NULL;
    }

    return png; 
}

IMC_Error imc_parse_png(png_hndl_t png) {
    IMC_Error status;
    chunk_t chunk = { 0 }; 
    ihdr_t  ihdr  = { 0 };
    idat_t  idat  = { 0 };
    uint8_t *out_buf = NULL;

    status = _imc_read_chunk(png, &chunk);
    if (status != IMC_EOK) {
        return status;
    }

    _imc_chunk_to_ihdr(&chunk, &ihdr);
#if 0
    _imc_print_ihdr_info(ihdr);
#endif
    _imc_destroy_chunk_data(&chunk);

    while (true) {
        if (_imc_read_chunk(png, &chunk) != IMC_EOK) {
            break;
        }
        /*if (_imc_proccess_next_chunk(png, &chunk) != IMC_EOK) {
            break;
        }*/
        _imc_chunk_to_idat(&ihdr, &chunk, &idat);

        out_buf = _imc_reconstruct_chunk_data(&ihdr, idat.decomp_buf, idat.length);
        if (_imc_destroy_chunk_data(&chunk) != IMC_EOK) {
            break;
        }
        break;
    }

    char iend_buf[4];
    char print_buf[5];
    /* There are 4 0 bytes after CRC for some reason */
    getc(png->fp);
    getc(png->fp);
    getc(png->fp);
    getc(png->fp);
    fread(iend_buf, sizeof(iend_buf), 1, png->fp);
    strncpy(print_buf, iend_buf, sizeof(iend_buf));
    print_buf[4] = '\0';
    printf("last 4 bytes: %s\n", print_buf);
    assert(memcmp(iend_buf, IEND, sizeof(iend_buf)) == 0);

    _output_raster(out_buf, ihdr.width, ihdr.height);

    free(idat.decomp_buf);
    free(out_buf);

    return IMC_EOK;
}

IMC_Error imc_close_png(png_hndl_t png) {
    int res = fclose(png->fp);
    if (res == -1) {
        IMC_WARN("Failed to close PNG");
        return IMC_ERROR;
    }
    return IMC_EOK;
}

IMC_Error imc_destroy_png(png_hndl_t png) {
    if (!png) {
        IMC_WARN("Attempted double free on PNG handle");
        return IMC_EFREE;
    }

    if (png->data) {
        free(png->data);
    }

    imc_close_png(png);
    free(png);
    png = NULL;

    return IMC_EOK;
}
