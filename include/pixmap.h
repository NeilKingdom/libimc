#ifndef PIXMAP_H
#define PIXMAP_H

#include "imc_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint8_t *data;      /* Raw data of the pixmap (agnostic to bitdepth and n channels) */
    size_t   offset;    /* Current read offset into the data buffer */
    size_t   width;     /* Width/stride of the image */
    size_t   height;    /* Height of the image */
} Pixmap_t;

typedef enum {
    NEAREST,    /* Nearest-neighbor */
    BILINEAR,   /* Bilinear interpolation */
    BICUBIC     /* Bicubic interpolation */
} ScaleMethod_t;

Rgb_t       imc_alpha_blend(const Rgb_t rgb, const float alpha, const Rgb_t bg_col);
Rgba_t      imc_sample_pixmap(const Pixmap_t pixmap, const float x, const float y);
Pixmap_t    imc_scale_pixmap(const Pixmap_t pixmap, const float width, const float height, const ScaleMethod_t sm);
Pixmap_t    imc_make_pixmap_grayscale(const Pixmap_t pixmap);
Pixmap_t    imc_make_pixmap_ascii_art(const Pixmap_t pixmap, const size_t ncols, const size_t nrows);
Pixmap_t    imc_make_pixmap_monochrome(const Pixmap_t pixmap, const float luma_threshold);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIXMAP_H */
