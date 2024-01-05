#ifndef PNG_PARSER_H
#define PNG_PARSER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PNG_MAGIC (uint8_t[8]){ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A }

typedef struct Png {
    FILE    *fp;    /* The file handle */
    uint8_t *data;  /* Raw bytes */
    size_t   size;  /* Size of data in bytes */
} *png_hndl_t;

png_hndl_t  imc_open_png(const char *path);
IMC_Error   imc_close_png(png_hndl_t png);
IMC_Error   imc_parse_png(png_hndl_t png);
IMC_Error   imc_destroy_png(png_hndl_t png);

#ifdef __cplusplus
}
#endif

#endif /* PNG_PARSER_H */
