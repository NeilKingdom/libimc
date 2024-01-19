/* Required for fileno() */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif 

#include <math.h>
#include <zlib.h>
#include <alloca.h>
#include <sys/stat.h> 

#include "../include/png_parser.h"

/**
 * @brief Flip the endianness of a given primitive
 * @since 15-01-2024
 * @param data The data whose endianness will be flipped
 */
#define _IMC_FLIP_ENDIAN(data) do { \
    size_t i; \
    uint8_t *tmp = alloca(sizeof(*(data))); \
    memcpy((void*)tmp, (void*)(data), sizeof(*(data))); \
    for (i = 0; i < sizeof(*(data)); ++i) { \
        ((uint8_t*)(data))[i] = tmp[sizeof(*(data)) - i - 1]; \
    } \
} while (0)

/**
 * @brief Get the size of a file in bytes
 * @since 15-01-2024
 * @param[in] fp File handle to the file that we want the size of
 * @returns Size of the file pointed to by fp
 */
static size_t _imc_get_file_size(FILE * restrict fp) {
    struct stat buf;
    int fd = fileno(fp);
    fstat(fd, &buf);
    return buf.st_size;
}

/**
 * @brief Validate the IHDR chunk of the PNG
 * @since 15-01-2024
 * @param[in] A png_hndl_t struct containing the relevant PNG image data
 * @returns IMC_ERROR if the file is either not a PNG or the PNG was corrupted
 *          or IMC_EOK upon success
 */
static IMC_Error _imc_validate_png_ihdr(const png_hndl_t restrict png) {
    int res = memcmp((void*)png->data, (void*)PNG_MAGIC, sizeof(PNG_MAGIC)); 
    if (res != 0) {
        IMC_WARN("Not a PNG file");
        return IMC_ERROR;
    }
    fseek(png->fp, sizeof(PNG_MAGIC), SEEK_SET); 

    return IMC_EOK;
}

/**
 *                    +-+-+
 * Previous scanline: |c|b|
 *                    +-+-+
 * Current scanline:  |a|x|
 *                    +-+-+
 */

/**
 * @brief IDAT reconstruct method 0 (NONE)
 * Recon(x) = Filt(x)
 * @since 15-01-2024
 * @param[in] prev_scanline The previously unfiltered scanline
 * @param[in/out] curr_scanline The scanline being reconstructed
 * @param[in] n_channels The number of channels / samples per pixel
 * @param[in] idx An index applied to curr_scanline to retrieve the current byte
 * @returns The reconstructed byte for curr_scanline[idx]
 */
static uint8_t _imc_recon_none(
    uint8_t *prev_scanline, 
    uint8_t *curr_scanline, 
    const uint8_t n_channels,
    const size_t idx
) {
    return curr_scanline[idx];
}

/**
 * @brief IDAT reconstruct method 1 (SUB)
 * Recon(x) = Filt(x) + Recon(a)
 * @since 15-01-2024
 * @param[in] prev_scanline The previously unfiltered scanline
 * @param[in/out] curr_scanline The scanline being reconstructed
 * @param[in] n_channels The number of channels / samples per pixel
 * @param[in] idx An index applied to curr_scanline to retrieve the current byte
 * @returns The reconstructed byte for curr_scanline[idx]
 */
static uint8_t _imc_recon_sub(
    uint8_t *prev_scanline, 
    uint8_t *curr_scanline, 
    const uint8_t n_channels,
    const size_t idx 
) {
    bool is_first = (idx < n_channels);
    uint32_t x = curr_scanline[idx];
    uint32_t a = (is_first) ? 0 : curr_scanline[idx - n_channels]; 
    uint8_t res = (x + a) % 256;

    curr_scanline[idx] = res;
    return res;
}

/**
 * @brief IDAT reconstruct method 2 (UP)
 * Recon(x) = Filt(x) + Recon(b)
 * @since 15-01-2024
 * @param[in] prev_scanline The previously unfiltered scanline
 * @param[in/out] curr_scanline The scanline being reconstructed
 * @param[in] n_channels The number of channels / samples per pixel
 * @param[in] idx An index applied to curr_scanline to retrieve the current byte
 * @returns The reconstructed byte for curr_scanline[idx]
 */
static uint8_t _imc_recon_up(
    uint8_t *prev_scanline, 
    uint8_t *curr_scanline, 
    const uint8_t n_channels,
    const size_t idx
) {
    uint32_t x = curr_scanline[idx];
    uint32_t b = prev_scanline[idx];
    uint8_t res = (x + b) % 256;

    curr_scanline[idx] = res;
    return res;
}

/**
 * @brief IDAT reconstruct method 3 (AVERAGE)
 * Recon(x) = Filt(x) + floor((Recon(a) + Recon(b)) / 2)
 * @since 15-01-2024
 * @param[in] prev_scanline The previously unfiltered scanline
 * @param[in/out] curr_scanline The scanline being reconstructed
 * @param[in] n_channels The number of channels / samples per pixel
 * @param[in] idx An index applied to curr_scanline to retrieve the current byte
 * @param[in] is_first Boolean indicating if the current byte resides within the first pixel of the scanline
 * @returns The reconstructed byte for curr_scanline[idx]
 */
static uint8_t _imc_recon_avg(
    uint8_t *prev_scanline, 
    uint8_t *curr_scanline, 
    const uint8_t n_channels,
    const size_t idx
) {
    bool is_first = (idx < n_channels);
    uint32_t x = curr_scanline[idx];
    uint32_t a = (is_first) ? 0 : curr_scanline[idx - n_channels];
    uint32_t b = prev_scanline[idx];
    uint8_t res = (x + (int)floorf(((float)a + (float)b) / 2.0f)) % 256;

    curr_scanline[idx] = res;
    return res;
}

/**
 * @brief The paeth predictor algorithm used in filter type 4: PAETH
 * @since 15-01-2024
 * @param a Byte to the left of x or 0 if x resides within the first pixel of the scanline
 * @param b Byte directly above x (same index of previous scanline) or 0 if parsing the first scanline
 * @param c Byte to the left of b or 0 if either x resides within the first pixel of the scanline of parsing 
 *          the first scanline
 * @returns The predictor for byte x
 */
static uint32_t _imc_peath_predictor(const uint8_t a, const uint8_t b, const uint8_t c) {
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

/**
 * @brief IDAT reconstruct method 4 (PAETH)
 * Recon(x) = Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c))
 * @since 15-01-2024
 * @param[in] prev_scanline The previously unfiltered scanline
 * @param[in/out] curr_scanline The scanline being reconstructed
 * @param[in] n_channels The number of channels / samples per pixel
 * @param[in] idx An index applied to curr_scanline to retrieve the current byte
 * @returns The reconstructed byte for curr_scanline[idx]
 */
static uint8_t _imc_recon_paeth(
    uint8_t *prev_scanline, 
    uint8_t *curr_scanline, 
    const uint8_t n_channels,
    const size_t idx
) {
    bool is_first = (idx < n_channels);
    uint32_t x = curr_scanline[idx];
    uint32_t a = (is_first) ? 0 : curr_scanline[idx - n_channels];
    uint32_t b = prev_scanline[idx];
    uint32_t c = (is_first) ? 0 : prev_scanline[idx - n_channels];
    uint8_t res = (x + _imc_peath_predictor(a, b, c)) % 256;

    curr_scanline[idx] = res;
    return res;
}

/**
 * @brief Read the next chunk from PNG and advance the file pointer
 * @since 15-01-2024
 * @param[in] A png_hndl_t struct containing the relevant PNG image data
 * @param[out] The output location for chunk information
 * @returns IMC_EFAULT if the function fails to allocate memory for chunk data, otherwise IMC_EOK
 */
static IMC_Error _imc_read_chunk(const png_hndl_t restrict png, chunk_t *chunk) {
    /*** Chunk length ***/
    
    fread((void*)&chunk->length, sizeof(chunk->length), 1, png->fp);
    _IMC_FLIP_ENDIAN(&chunk->length);

    /*** Chunk type ***/

    fread((void*)&chunk->type, sizeof(chunk->type), 1, png->fp);
    /* Check if at IEND */
    if (memcmp((void*)chunk->type, IEND, sizeof(chunk->type)) == 0) {
        return IMC_ERROR; /* TODO: Not the best error to return... */
    }

    /*** Chunk data ***/

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

    /*** Chunk CRC ***/

    fread((void*)&chunk->crc, sizeof(chunk->crc), 1, png->fp);
    _IMC_FLIP_ENDIAN(&chunk->crc);

    return IMC_EOK;
}

/**
 * @brief Frees the data field of chunk
 * @since 15-01-2024
 * @param[in] The chunk whos data shall be destroyed
 * @returns IMC_EINVAL if the chunk is NULL, otherwise IMC_EOK
 */
static IMC_Error _imc_destroy_chunk_data(chunk_t *chunk) {
    if (!chunk) {
        IMC_WARN("Attempted double free on chunk");
        return IMC_EINVAL;
    }

    if (chunk->data) {
        free(chunk->data);
        chunk->data = NULL;
    }

    return IMC_EOK;
}

/**
 * @brief Converts raw chunk data to an ihdr_t struct
 * @since 15-01-2024
 * @param[in] chunk The chunk whos data will be converted to an ihdr_t
 * @param[out] The output location for the converted chunk data
 * @warning May fail if PNG ever supports compression methods beyond 0 (deflate) or filter methods beyond 0
 */
static void _imc_chunk_to_ihdr(const chunk_t * restrict chunk, ihdr_t * restrict ihdr) {
    memcpy((void*)ihdr, (void*)chunk->data, sizeof(*ihdr));
    _IMC_FLIP_ENDIAN(&ihdr->width);
    _IMC_FLIP_ENDIAN(&ihdr->height);
    _IMC_FLIP_ENDIAN(&ihdr->bit_depth);
    _IMC_FLIP_ENDIAN(&ihdr->color_type);

    switch (ihdr->color_type) {
        case NONE: 
            IMC_WARN("GREYSCALE not implemented");
            assert(false);
            break;
        case COLOR: 
            ihdr->n_channels = 3;
            break;
        case ALPHA: 
            IMC_WARN("ALPHA not implemented");
            assert(false);
            break;
        case (PALETTE | COLOR):
            IMC_WARN("(PALETTE | COLOR) not implemented");
            assert(false);
            break;
        case (COLOR | ALPHA):
            ihdr->n_channels = 4;
            break;
    }

    _IMC_FLIP_ENDIAN(&ihdr->compress_mthd);
    /* PNG currently supports type 0 only */
    assert(ihdr->compress_mthd == 0); 
    _IMC_FLIP_ENDIAN(&ihdr->filter_mthd);
    /* PNG currently supports filter method 0 only */
    assert(ihdr->filter_mthd == 0); 
    _IMC_FLIP_ENDIAN(&ihdr->interlace_mthd);
}

/**
 * @brief Print IHDR debug information 
 * @since 15-01-2024
 * @param[in] ihdr The ihdr_t struct whos data will be logged
 * @warning Compression method and filter method will be hardcoded until PNG supports alternative standards
 */
static void _imc_print_ihdr_info(const ihdr_t ihdr) {
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
        case ALPHA: 
            printf("Greyscale + Alpha\n");
            break;
        case (PALETTE | COLOR):
            printf("Palette\n");
            break;
        case (COLOR | ALPHA):
            printf("RGBA\n");
            break;
    }
    printf("Compression method: Deflate\n");
    printf("Filter method: 0\n");
    printf("Interlaced: %s\n", (ihdr.interlace_mthd == 0) ? "False" : "True");
}

static IMC_Error _imc_append_idat(const chunk_t * restrict chunk, idat_t * restrict idat) {
    idat->length += chunk->length;
    idat->data = realloc(idat->data, idat->length);
    if (idat->data == NULL) {
        IMC_WARN("Failed to allocate memory for IDAT compression data");
        return IMC_EFAULT;
    }
    memcpy((void*)(idat->data + idat->offset), (void*)chunk->data, chunk->length);
    idat->offset = idat->length;

    return IMC_EOK;
}

/**
 * @brief Converts raw chunk data to an idat_t struct using zlib's decompression algorithm
 * @param[in] ihdr The IHDR information pertaining to the PNG
 * @param[in] chunk A chunk containing compressed IDAT data
 * @param[out] idat The struct to which the decompressed IDAT data will be copied
 * @returns IMC_EFAULT if an allocation fails or IMC_ERROR for zlib errors, otherwise returns IMC_EOK
 */
static IMC_Error _imc_decompress_idat(
    const ihdr_t * restrict ihdr, 
    const idat_t * restrict idat, 
    uint8_t **decomp_buf
) {
    int res;
    z_stream stream;
    uint8_t n_channels;
    size_t scanline_len, decomp_len;

    /* Ensure compression method is set as deflate */
    assert((idat->data[0] & 0x0F) == 0x08);

    scanline_len = ((ihdr->n_channels * ihdr->width * ihdr->bit_depth + 7) >> 3) + 1; /* +1 for filter type */
    decomp_len = scanline_len * ihdr->height;
    *decomp_buf = malloc(decomp_len);
    if (*decomp_buf == NULL) {
        IMC_WARN("Failed to allocate memory for decompression buffer");
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
        free(*decomp_buf);
        *decomp_buf = NULL;
        return IMC_ERROR;
    }

    stream.avail_in = idat->length;

    do {
        if (stream.avail_in == 0) break;
        stream.next_in = idat->data;

        do {
            stream.avail_out = decomp_len;
            stream.next_out  = *decomp_buf;

            res = inflate(&stream, Z_NO_FLUSH);
            switch (res) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                {
                    IMC_WARN("Decompression error");
                    (void)inflateEnd(&stream);
                    free(*decomp_buf);
                    *decomp_buf = NULL;
                    return res;
                }
            }
        } while (stream.avail_out == 0);
    } while (res != Z_STREAM_END);

    /* Cleanup */
    (void)inflateEnd(&stream);
    return (res == Z_STREAM_END) ? IMC_EOK : IMC_ERROR;
}

/**
 *
 */
static IMC_Error _imc_reconstruct_idat(
    const ihdr_t * restrict ihdr, 
    uint8_t * restrict decomp_buf,
    pixmap_t *pixmap
) {
    int res;
    size_t x, scanline_len;
    size_t decomp_off;
    uint8_t fm, n_channels;
    uint8_t *curr_scanline = NULL;
    uint8_t *prev_scanline = NULL;
    recon_func rf;

    pixmap->width = scanline_len = (ihdr->n_channels * ihdr->width * ihdr->bit_depth + 7) >> 3;
    pixmap->height = ihdr->height;
    pixmap->data = malloc(pixmap->width * pixmap->height);
    if (pixmap->data == NULL) {
        IMC_WARN("Failed to allocate memory for pixmap->data");
        return IMC_EFAULT;
    }

    prev_scanline = alloca(scanline_len);
    memset((void*)prev_scanline, 0, scanline_len);

    for (decomp_off = 0; decomp_off < (pixmap->width * pixmap->height); decomp_off += scanline_len) {
        /* Filter method */
        fm = decomp_buf[decomp_off++];
        assert(fm <= 4);

        switch (fm) {
            case NONE: 
                rf = _imc_recon_none;
                break;                
            case SUB:
                rf = _imc_recon_sub;
                break;
            case UP:
                rf = _imc_recon_up;
                break;
            case AVG: 
                rf = _imc_recon_avg;
                break;
            case PAETH: 
                rf = _imc_recon_paeth;
                break;
        }

        curr_scanline = decomp_buf + decomp_off;

        for (x = 0; x < scanline_len; ++x) {
            pixmap->data[pixmap->offset++] = rf(prev_scanline, curr_scanline, ihdr->n_channels, x);
        }

        prev_scanline = curr_scanline;
    }

    return IMC_EOK;
}

static void _imc_chunk_to_plte(chunk_t *chunk, plte_t *plte) {

}

static void _imc_chunk_to_chrm(chunk_t *chunk, chrm_t *chrm) {

}

static void _imc_chunk_to_gama(chunk_t *chunk, gama_t *gama) {

}

static void _imc_chunk_to_iccp(chunk_t *chunk, iccp_t *iccp) {

}

/*static void _imc_chunk_to_sbit(chunk_t *chunk, sbit_t *sbit) {

}

static void _imc_chunk_to_srgb(chunk_t *chunk, srgb_t *srgb) {

}

static void _imc_chunk_to_bkgd(chunk_t *chunk, bkgd_t *bkgd) {

}

static void _imc_chunk_to_hist(chunk_t *chunk, hist_t *hist) {

}*/

static void _imc_chunk_to_trns(chunk_t *chunk, trns_t *trns) {

}

/*static IMC_Error _imc_process_next_chunk(png_hndl_t png, chunk_t *chunk) {
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
}*/

static rgb_t _imc_alpha_blend(const rgb_t rgb, const float alpha, const rgb_t bg_col) {
    uint8_t r, g, b;
    r = (1.0f - alpha) * bg_col.r + alpha * rgb.r;
    g = (1.0f - alpha) * bg_col.g + alpha * rgb.g;
    b = (1.0f - alpha) * bg_col.b + alpha * rgb.b;
    return (rgb_t){ r, g, b };
}

/**
 * @brief Outputs a PPM file containing the pixmap's data 
 * @since 15-01-2024
 * @param[in] fname The name of the output file
 * @param[in] pixmap A pixmap_t struct containing the data to be written to the PPM file
 */
static void _write_ppm_file(const ihdr_t * restrict ihdr, const char *fname, const pixmap_t pixmap) {
    rgb_t bg_col = (rgb_t){ 255, 255, 255 };
    size_t x, y, bpp, scanline_len;
    FILE *fp = NULL;
    
    bpp = (size_t)powf(2.0f, ihdr->bit_depth) - 1.0f;
    scanline_len = pixmap.width / ihdr->n_channels;
    fp = fopen(fname, "wb");

    /* Header */
    fputs("P6\n", fp);
    fprintf(fp, "%ld %ld\n", scanline_len, pixmap.height);
    fprintf(fp, "%ld\n", bpp);
     
    /* Data */
    for (y = 0; y < pixmap.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            if (ihdr->color_type == COLOR) {
                rgb_t rgb = ((rgb_t*)pixmap.data)[(y * scanline_len) + x];
                fputc(rgb.r, fp);
                fputc(rgb.g, fp);
                fputc(rgb.b, fp);
            } else if (ihdr->color_type == (COLOR | ALPHA)) {
                rgba_t rgba = ((rgba_t*)pixmap.data)[(y * scanline_len) + x];
                rgb_t rgb = _imc_alpha_blend((rgb_t){ rgba.r, rgba.g, rgba.b }, rgba.a, bg_col);
                fputc(255 - rgb.r, fp);
                fputc(255 - rgb.g, fp);
                fputc(255 - rgb.b, fp);
            }
        }
    }

    fclose(fp);
}

/**
 * @brief Prepare/open a PNG file for parsing 
 * @since 15-01-2024
 * @param path An absolute or relative path to the PNG file
 * @returns A png_hndl_t struct which can be passed to other functions of the library
 */
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
    if (_imc_validate_png_ihdr(png) != IMC_EOK) {
        return NULL;
    }

    return png; 
}

static bool _imc_chunk_is_type(const chunk_t * restrict chunk, const char *type) {
    if (memcmp(chunk->type, type, sizeof(chunk->type)) == 0) {
        return true;
    } else {
        return false;
    }
}

IMC_Error imc_parse_png(png_hndl_t png) {
    IMC_Error status;
    chunk_t chunk = { 0 }; 
    ihdr_t  ihdr  = { 0 };
    idat_t  idat  = { 0 };
    pixmap_t pixmap = { 0 };
    uint8_t *decomp_buf = NULL;

    /* Read PNG IHDR chunk */
    _imc_read_chunk(png, &chunk);
    _imc_chunk_to_ihdr(&chunk, &ihdr);
    _imc_print_ihdr_info(ihdr);
    _imc_destroy_chunk_data(&chunk);

    /* Read all ancillary chunks prior to IDAT */
    while (true) {
        _imc_read_chunk(png, &chunk);
        if (!_imc_chunk_is_type(&chunk, IDAT)) {
            _imc_destroy_chunk_data(&chunk);
        } else {
            break;
        }
    } 

    /* Read IDAT chunks */
    while (true) {
        if (_imc_chunk_is_type(&chunk, IDAT)) {
            _imc_append_idat(&chunk, &idat);
            _imc_destroy_chunk_data(&chunk);
        } else {
            break;
        }
        _imc_read_chunk(png, &chunk);
    } 

    _imc_decompress_idat(&ihdr, &idat, &decomp_buf);
    _imc_reconstruct_idat(&ihdr, decomp_buf, &pixmap);

#ifdef DEBUG
    _write_ppm_file(&ihdr, "raster.ppm", pixmap);
#endif

    free(idat.data);
    free(pixmap.data);

    return IMC_EOK;
}

IMC_Error imc_close_png(png_hndl_t png) {
    int res;

    if (!png) {
        IMC_WARN("Attempted double free on PNG handle");
        return IMC_EFREE;
    }

    if (png->data) {
        free(png->data);
    }

    if (png->fp) {
        res = fclose(png->fp);
        if (res == -1) {
            IMC_WARN("Failed to close PNG");
            return IMC_ERROR;
        }
    }

    free(png);
    png = NULL;

    return IMC_EOK;
}
