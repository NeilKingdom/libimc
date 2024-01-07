/* Required for fileno() */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif 

#include <alloca.h>
#include <stdbool.h>
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

static IMC_Error _imc_read_chunk(png_hndl_t png, chunk_t *chunk) {
    /* Chunk length */
    fread((void*)&chunk->length, sizeof(chunk->length), 1, png->fp);
    _IMC_FLIP_ENDIAN(&chunk->length);

    /* Chunk type */
    fread((void*)&chunk->type, sizeof(chunk->type), 1, png->fp);

    /* Chunk data */
    chunk->data = malloc(chunk->length);
    if (chunk->data == NULL) {
        IMC_WARN("Failed to allocate space for chunk data");
        return IMC_EFAULT;
    }
    fread((void*)chunk->data, chunk->length, 1, png->fp);

    /* Chunk CRC */
    fread((void*)&chunk->crc, sizeof(chunk->crc), 1, png->fp);

    return IMC_EOK;
}

static void _imc_get_ihdr_from_chunk(chunk_t *chunk, ihdr_t *ihdr) {
    assert(chunk->length + sizeof(ihdr->padding) == sizeof(*ihdr)); 
    memcpy((void*)ihdr, (void*)chunk->data, sizeof(*ihdr));
    _IMC_FLIP_ENDIAN(&ihdr->width);
    _IMC_FLIP_ENDIAN(&ihdr->height);
    _IMC_FLIP_ENDIAN(&ihdr->bit_depth);
    _IMC_FLIP_ENDIAN(&ihdr->color_type);
    _IMC_FLIP_ENDIAN(&ihdr->compress_mthd);
    _IMC_FLIP_ENDIAN(&ihdr->filter_mthd);
    _IMC_FLIP_ENDIAN(&ihdr->interlace_mthd);
}

static void _imc_print_ihdr_info(ihdr_t ihdr) {
    printf("Image width: %d\n", ihdr.width);
    printf("Image height: %d\n", ihdr.height);
    printf("Pixel bit-depth: %d\n", ihdr.bit_depth);
    printf("Color type: ");
    switch (ihdr.color_type) {
        case NONE: 
            printf("Grayscale\n");
        case COLOR: 
            printf("RGB\n");
            break;
        case (PALETTE | COLOR):
            printf("Palette\n");
        case ALPHA: 
            printf("Grayscale + Alpha\n");
            break;
        case (COLOR | ALPHA):
            printf("RGB + Alpha\n");
            break;
        default:
            printf("Invalid\n");
            break;
    }
    printf("file compression method: %d\n", ihdr.compress_mthd);
    printf("file filter method: %d\n", ihdr.filter_mthd);
    printf(
        "file interlace method: %s\n", 
        (ihdr.interlace_mthd == 0) ? "Non-interlaced" : "Interlaced"
    );
}

png_hndl_t imc_open_png(const char *path) {
    png_hndl_t png = NULL;
    FILE *fp = NULL;

    /* Allocate PNG handle */
    png = malloc(sizeof(png_hndl_t));
    if (!png) {
        IMC_WARN("Failed to allocate memory for png");
        return NULL;
    }

    /* Open PNG file */
    fp = fopen(path, "rw");
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

    status = _imc_read_chunk(png, &chunk);
    if (status != IMC_EOK) {
        return status;
    }

    _imc_get_ihdr_from_chunk(&chunk, &ihdr);
#ifdef DEBUG
    _imc_print_ihdr_info(ihdr);
#endif

    

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
