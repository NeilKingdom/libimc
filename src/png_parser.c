/* Required for fileno() */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif 

#include <sys/stat.h> 
#include "../include/png_parser.h"

static size_t _imc_get_file_size(FILE *fp) {
    struct stat buf;
    int fd = fileno(fp);
    fstat(fd, &buf);
    return buf.st_size;
}

static void _imc_validate_png(png_hndl_t png) {
    /* Check MAGIC */
    int res = memcmp((void*)png->data, (void*)PNG_MAGIC, 8); 
    if (res != 0) {
        IMC_WARN("Not a PNG file");
        exit(EXIT_FAILURE);
    }
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
    _imc_validate_png(png);
    return png; 
}

IMC_Error imc_close_png(png_hndl_t png) {
    int res = fclose(png->fp);
    if (res == -1) {
        IMC_WARN("Failed to close PNG");
        return IMC_ERROR;
    }
    return IMC_EOK;
}

IMC_Error imc_parse_png(png_hndl_t png) {
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
