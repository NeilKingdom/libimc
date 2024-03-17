#ifndef PIXMAP_H
#define PIXMAP_H

#include "imc_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint8_t *data;
    size_t   offset;
    size_t   width;
    size_t   height;
} Pixmap_t;

typedef enum {
    NEAREST,    /* Nearest-neighbor */
    BILINEAR,   /* Bilinear interpolation */
    BICUBIC     /* Bicubic interpolation */
} ScaleMethod_t;

Rgba_t      imc_sample_pixbuf(const Pixmap_t pixmap, const float x, const float y);
Pixmap_t    imc_scale_pixbuf(const Pixmap_t pixmap, const float width, const float height, const ScaleMethod_t sm);
Pixmap_t    imc_make_pixbuf_grayscale(const Pixmap_t pixmap);
Pixmap_t    imc_make_pixbuf_monochrome(const Pixmap_t pixmap, const float luma_threshold);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIXMAP_H */
