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

pixmap_t    imc_scale_pixbuf(const pixmap_t pixmap, const float new_width, const float new_height, const ScaleMethod sm);
rgb_t       imc_sample_pixbuf(const pixmap_t pixmap, const float x, const float y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIXMAP_H */
