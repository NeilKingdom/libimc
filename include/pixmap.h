#ifndef PIXMAP_H
#define PIXMAP_H

#include "imc_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint8_t *data;       /* Raw data of the pixmap (agnostic to bitdepth and # of channels) */
    uint8_t  n_channels; /* Number of color channels */
    uint8_t  bit_depth;  /* Bits per pixel */
    size_t   offset;     /* Current read offset into the data buffer */
    size_t   width;      /* Width/stride of the image */
    size_t   height;     /* Height of the image */
} Pixmap_t;

typedef enum {
    NEAREST,    /* Nearest-neighbor */
    BILINEAR,   /* Bilinear interpolation */
    BICUBIC     /* Bicubic interpolation */
} ScaleMethod_t;

/* Forward function declarations */

Rgb_t       imc_blend_alpha(const Rgb_t fg_col, const Rgb_t bg_col, const float alpha);
Rgba_t      imc_pixmap_sample(Pixmap_t *pixmap, const float x, const float y);
ImcError_t  imc_pixmap_scale(Pixmap_t *pixmap, const size_t width, const size_t height, const ScaleMethod_t sm);
ImcError_t  imc_pixmap_to_grayscale(Pixmap_t *pixmap);
ImcError_t  imc_pixmap_to_monochrome(Pixmap_t *pixmap, const float luma_threshold);
ImcError_t  imc_pixmap_to_ascii(Pixmap_t *pixmap, const char* const fname);
ImcError_t  imc_pixmap_to_ppm(Pixmap_t *pixmap, const char* const fname, const Rgb_t bg_col);
ImcError_t  imc_pixmap_rotate_cw(Pixmap_t *pixmap);
ImcError_t  imc_pixmap_rotate_ccw(Pixmap_t *pixmap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIXMAP_H */
