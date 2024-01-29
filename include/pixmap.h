#ifndef PIXMAP_H
#define PIXMAP_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct PixBuf {
    uint8_t *data;
    size_t   offset;
    size_t   width;
    size_t   height;
} pixmap_t;

typedef enum {
    NEAREST,    /* Nearest-neighbor */
    BILINEAR,   /* Bilinear interpolation */
    BICUBIC     /* Bicubic interpolation */
} ScaleMethod;

rgba_t      imc_sample_pixbuf(const pixmap_t pixmap, const float x, const float y);
pixmap_t    imc_scale_pixbuf(const pixmap_t pixmap, const float width, const float height, const ScaleMethod sm);
pixmap_t    imc_make_pixbuf_grayscale(const pixmap_t pixmap);
pixmap_t    imc_make_pixbuf_monochrome(const pixmap_t pixmap, const float luma_threshold);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIXMAP_H */
