/**
 * @file png_parser.c
 * @author Neil Kingdom
 * @since 15-01-2024
 * @version 1.0
 * @brief Contains the internal functions necessary for parsing a PNG file.
 */

#include "png_parser.h"

/*
 * ===============================
 *       Private Functions
 * ===============================
 */

/**
 * @brief Flip the endianness of a given primitive.
 * @since 15-01-2024
 * @param[in] data The data whose endianness will be flipped
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
 * @brief Checks to see if a chunk matches the given type.
 * @since 15-01-2024
 * @param chunk The chunk who's type is being checked
 * @param type The type that we're checking for
 * @returns True if the chunk is of type "type", or false otherwise
 */
static bool _imc_chunk_is_type(
    const Chunk_t* const chunk,
    const char* const type
) {
    if (memcmp(chunk->type, type, sizeof(chunk->type)) == 0) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Get the size of a file in bytes.
 * @since 15-01-2024
 * @param fp File handle to the file that we want the size of
 * @returns Size of the file pointed to by fp
 */
static size_t _imc_get_file_size(FILE *fp) {
    struct stat buf;
    int fd = fileno(fp);
    fstat(fd, &buf);
    return buf.st_size;
}

/**
 * @brief Validate the IHDR chunk of the PNG and seek past it.
 * @since 15-01-2024
 * @param A png_hndl_t struct containing the relevant PNG image data
 * @returns IMC_ERROR if the file is either not a PNG or the PNG was corrupted
 *          or IMC_EOK upon success
 */
static ImcError_t _imc_validate_png_ihdr(const PngHndl_t* const png) {
    int status = memcmp((void*)png->data, (void*)PNG_MAGIC, sizeof(PNG_MAGIC));
    if (status != 0) {
        IMC_LOG("Not a PNG file", IMC_ERROR);
        return IMC_EFAIL;
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
 * @brief IDAT reconstruct method 0 (NONE).
 * Recon(x) = Filt(x)
 * @since 15-01-2024
 * @param prev_scanline The previously unfiltered scanline
 * @param curr_scanline The scanline being reconstructed
 * @param n_channels The number of channels / samples per pixel
 * @param idx An index applied to curr_scanline to retrieve the current byte
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
 * @brief IDAT reconstruct method 1 (SUB).
 * Recon(x) = Filt(x) + Recon(a)
 * @since 15-01-2024
 * @param prev_scanline The previously unfiltered scanline
 * @param curr_scanline The scanline being reconstructed
 * @param n_channels The number of channels / samples per pixel
 * @param idx An index applied to curr_scanline to retrieve the current byte
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
 * @brief IDAT reconstruct method 2 (UP).
 * Recon(x) = Filt(x) + Recon(b)
 * @since 15-01-2024
 * @param prev_scanline The previously unfiltered scanline
 * @param curr_scanline The scanline being reconstructed
 * @param n_channels The number of channels / samples per pixel
 * @param idx An index applied to curr_scanline to retrieve the current byte
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
 * @brief IDAT reconstruct method 3 (AVERAGE).
 * Recon(x) = Filt(x) + floor((Recon(a) + Recon(b)) / 2)
 * @since 15-01-2024
 * @param prev_scanline The previously unfiltered scanline
 * @param curr_scanline The scanline being reconstructed
 * @param n_channels The number of channels / samples per pixel
 * @param idx An index applied to curr_scanline to retrieve the current byte
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
 * @brief The paeth predictor algorithm used in filter type 4: PAETH.
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
 * @brief IDAT reconstruct method 4 (PAETH).
 * Recon(x) = Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c))
 * @since 15-01-2024
 * @param prev_scanline The previously unfiltered scanline
 * @param curr_scanline The scanline being reconstructed
 * @param n_channels The number of channels / samples per pixel
 * @param idx An index applied to curr_scanline to retrieve the current byte
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
 * @brief Read the next chunk from PNG and advance the file pointer.
 * @since 15-01-2024
 * @param A png_hndl_t struct containing the relevant PNG image data
 * @param The output location for chunk information
 * @returns IMC_EFAULT if the function fails to allocate memory for chunk data, otherwise IMC_EOK
 */
static ImcError_t _imc_read_chunk(const PngHndl_t* const png, Chunk_t *chunk) {
    /*** Chunk length ***/

    fread((void*)&chunk->length, sizeof(chunk->length), 1, png->fp);
    _IMC_FLIP_ENDIAN(&chunk->length);

    /*** Chunk type ***/

    fread((void*)&chunk->type, sizeof(chunk->type), 1, png->fp);
    /* Check if at IEND */
    if (memcmp((void*)chunk->type, IEND, sizeof(chunk->type)) == 0) {
        return IMC_EFAIL; /* TODO: Not the best error to return... */
    }

    /*** Chunk data ***/

    if (chunk->length == 0) {
        chunk->data = NULL;
    } else {
        chunk->data = malloc(chunk->length);
        if (chunk->data == NULL) {
            IMC_LOG("Failed to allocate space for chunk data", IMC_ERROR);
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
 * @brief Frees the data field of chunk.
 * @since 15-01-2024
 * @param The chunk whos data shall be destroyed
 * @returns IMC_EINVAL if the chunk is NULL, otherwise IMC_EOK
 */
static ImcError_t _imc_destroy_chunk_data(Chunk_t *chunk) {
    if (chunk == NULL) {
        IMC_LOG("Attempted double free on chunk", IMC_WARNING);
        return IMC_EINVAL;
    }

    if (chunk->data) {
        free(chunk->data);
        chunk->data = NULL;
    }

    return IMC_EOK;
}

/**
 * @brief Converts raw chunk data to an ihdr_t struct.
 * @since 15-01-2024
 * @param chunk The chunk whos data will be converted to an ihdr_t
 * @param The output location for the converted chunk data
 * @warning May fail if PNG ever supports compression methods beyond 0 (deflate) or filter methods beyond 0
 */
static void _imc_chunk_to_ihdr(const Chunk_t* const chunk, Ihdr_t *ihdr) {
    memcpy((void*)ihdr, (void*)chunk->data, sizeof(*ihdr));
    _IMC_FLIP_ENDIAN(&ihdr->width);
    _IMC_FLIP_ENDIAN(&ihdr->height);
    _IMC_FLIP_ENDIAN(&ihdr->bit_depth);
    _IMC_FLIP_ENDIAN(&ihdr->color_type);

    switch (ihdr->color_type) {
        case NONE:
            IMC_LOG("GREYSCALE not implemented", IMC_ERROR);
            exit(EXIT_FAILURE);
            break;
        case COLOR:
            ihdr->n_channels = 3;
            break;
        case ALPHA:
            IMC_LOG("ALPHA not implemented", IMC_ERROR);
            exit(EXIT_FAILURE);
            break;
        case (PALETTE | COLOR):
            IMC_LOG("(PALETTE | COLOR) not implemented", IMC_ERROR);
            exit(EXIT_FAILURE);
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
 * @brief Print IHDR debug information.
 * @since 15-01-2024
 * @param ihdr The ihdr_t struct whos data will be logged
 * @warning Compression method and filter method will be hardcoded until PNG supports alternative standards
 */
static void _imc_print_ihdr_info(const Ihdr_t ihdr) {
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

/**
 * @brief Appends the chunk onto the IDAT's data segment.
 * @since 15-01-2024
 * @param chunk The chunk that will be appended to the IDAT's data segment
 * @param idat The IDAT struct onto which the chunk shall be appended
 * @returns IMC_EFAULT if reallocation of IDAT's data fails, otherwise returns IMC_EOK
 */
static ImcError_t _imc_append_idat(const Chunk_t* const chunk, Idat_t *idat) {
    idat->length += chunk->length;
    idat->data = realloc(idat->data, idat->length);
    if (idat->data == NULL) {
        IMC_LOG("Failed to allocate memory for IDAT compression data", IMC_ERROR);
        return IMC_EFAULT;
    }
    memcpy((void*)(idat->data + idat->offset), (void*)chunk->data, chunk->length);
    idat->offset = idat->length;

    return IMC_EOK;
}

/**
 * @brief Decompresses the IDAT's compressed data stream using the LZ77 algorithm.
 * @since 15-01-2024
 * @param ihdr The IHDR information pertaining to the PNG
 * @param idat The IDAT chunk containing the compressed data
 * @param decomp_buf A raw buffer into which the decompressed data will be copied
 * @returns IMC_EFAULT if an allocation fails or IMC_ERROR for zlib errors, otherwise returns IMC_EOK
 */
static ImcError_t _imc_decompress_idat(
    const Ihdr_t *ihdr,
    const Idat_t *idat,
    uint8_t **decomp_buf
) {
    int status;
    z_stream stream;
    uint8_t n_channels;
    size_t scanline_len, decomp_len;

    /* Ensure compression method is set as deflate */
    assert((idat->data[0] & 0x0F) == 0x08);

    scanline_len = ((ihdr->n_channels * ihdr->width * ihdr->bit_depth + 7) >> 3) + 1; /* +1 for filter type */
    decomp_len = scanline_len * ihdr->height;
    *decomp_buf = malloc(decomp_len);
    if (*decomp_buf == NULL) {
        IMC_LOG("Failed to allocate memory for decompression buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    stream.zalloc   = Z_NULL;
    stream.zfree    = Z_NULL;
    stream.opaque   = Z_NULL;
    stream.avail_in = 0;
    stream.next_in  = Z_NULL;

    status = inflateInit(&stream);
    if (status != Z_OK) {
        IMC_LOG("Failed to initialize decompression stream", IMC_ERROR);
        free(*decomp_buf);
        *decomp_buf = NULL;
        return IMC_EFAIL;
    }

    stream.avail_in = idat->length;

    do {
        if (stream.avail_in == 0) break;
        stream.next_in = idat->data;

        do {
            stream.avail_out = decomp_len;
            stream.next_out  = *decomp_buf;

            status = inflate(&stream, Z_NO_FLUSH);
            switch (status) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                {
                    IMC_LOG("Decompression error", IMC_ERROR);
                    (void)inflateEnd(&stream);
                    free(*decomp_buf);
                    *decomp_buf = NULL;
                    return status;
                }
            }
        } while (stream.avail_out == 0);
    } while (status != Z_STREAM_END);

    /* Cleanup */
    (void)inflateEnd(&stream);
    return (status == Z_STREAM_END) ? IMC_EOK : IMC_EFAIL;
}

/**
 * @brief Reconstructs the IDAT chunk after it's been decompressed.
 * @since 15-01-2024
 * @param ihdr The IHDR information pertaining to the PNG
 * @param decomp_buf The decompressed buffer data of the IDAT
 * @param pixmap A Pixmap_t struct into which the reconstructed data will be copied
 * @returns An ImcError_t indicating the exit status code
 */
static ImcError_t _imc_reconstruct_idat(
    const Ihdr_t* const ihdr,
    uint8_t *decomp_buf,
    Pixmap_t *pixmap
) {
    int status;
    size_t x, scanline_len, decomp_off;
    uint8_t fm, n_channels;
    uint8_t *curr_scanline = NULL;
    uint8_t *prev_scanline = NULL;
    recon_func rf;

    pixmap->width = scanline_len = (ihdr->n_channels * ihdr->width * ihdr->bit_depth + 7) >> 3;
    pixmap->height = ihdr->height;
    pixmap->bit_depth = ihdr->bit_depth;
    pixmap->n_channels = ihdr->n_channels;
    pixmap->data = malloc(pixmap->width * pixmap->height);
    if (pixmap->data == NULL) {
        IMC_LOG("Failed to allocate memory for pixmap->data", IMC_ERROR);
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

/*
static void _imc_chunk_to_plte(Chunk_t *chunk, Plte_t *plte) {

}

static void _imc_chunk_to_chrm(Chunk_t *chunk, Chrm_t *chrm) {

}

static void _imc_chunk_to_gama(Chunk_t *chunk, Gama_t *gama) {

}

static void _imc_chunk_to_iccp(Chunk_t *chunk, Iccp_t *iccp) {

}

static void _imc_chunk_to_sbiT(chunk_t *chunk, Sbit_t *sbit) {

}

static void _imc_chunk_to_srgb(Chunk_t *chunk, Srgb_t *srgb) {

}

static void _imc_chunk_to_bkgd(Chunk_t *chunk, Bkgd_t *bkgd) {

}

static void _imc_chunk_to_hist(Chunk_t *chunk, Hist_t *hist) {

}

static void _imc_chunk_to_trns(Chunk_t *chunk, Trns_t *trns) {

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

/*
 * ===============================
 *       Public Functions
 * ===============================
 */

/**
 * @brief Prepare/open a PNG file for parsing.
 * @since 15-01-2024
 * @param path An absolute or relative path to the PNG file
 * @returns A png_hndl_t struct which can be passed to other functions of the library
 */
PngHndl_t *imc_png_open(const char* const path) {
    PngHndl_t *png = NULL;
    FILE *fp = NULL;

    png = malloc(sizeof(PngHndl_t));
    if (png == NULL) {
        IMC_LOG("Failed to allocate memory for png", IMC_ERROR);
        return NULL;
    }

    fp = fopen(path, "r+");
    if (fp == NULL) {
        IMC_LOG("Failed to open file", IMC_ERROR);
        return NULL;
    }

    png->fp = fp;
    png->size = _imc_get_file_size(fp);
    png->data = malloc(png->size);
    if (png->data == NULL) {
        IMC_LOG("Failed to allocate memory for png data", IMC_ERROR);
        return NULL;
    }

    fread(png->data, png->size, 1, png->fp);
    if (_imc_validate_png_ihdr(png) != IMC_EOK) {
        return NULL;
    }

    return png;
}

/**
 * @brief Parse a PNG image and return it as a Pixmap_t structure.
 * @since 15-01-2024
 * @param png A handle to the png file obtained via imc_open_png()
 * @returns A Pixmap_t structure containing the raw PNG pixel data
 */
Pixmap_t *imc_png_parse(PngHndl_t *png) {
    ImcError_t status;
    Chunk_t chunk = { 0 };
    Ihdr_t  ihdr  = { 0 };
    Idat_t  idat  = { 0 };
    Pixmap_t *pixmap = NULL;
    uint8_t *decomp_buf = NULL;

    pixmap = malloc(sizeof(Pixmap_t));
    if (pixmap == NULL) {
        IMC_LOG("Failed to allocate memory for Pixmap_t", IMC_ERROR);
        return NULL;
    }

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
    _imc_reconstruct_idat(&ihdr, decomp_buf, pixmap);

    free(idat.data);
    return pixmap;
}

/**
 * @brief Close the PNG file referenced by "png".
 * @since 15-01-2024
 * @param png A PngHndl_t pointer refererencing the PNG file to be closed
 * @returns An ImcError_t indicating the exit status of the operation
 */
ImcError_t imc_png_close(PngHndl_t *png) {
    int res;

    if (png == NULL) {
        IMC_LOG("Attempted double free on PNG handle", IMC_WARNING);
        return IMC_EFAULT;
    }

    if (png->data) {
        free(png->data);
    }

    if (png->fp) {
        res = fclose(png->fp);
        if (res == -1) {
            IMC_LOG("Failed to close PNG", IMC_WARNING);
            return IMC_EFAIL;
        }
    }

    free(png);
    png = NULL;

    return IMC_EOK;
}
