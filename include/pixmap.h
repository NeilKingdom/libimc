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

/* TODO: Change naming to imc_pixmap_thing */
/* TODO: All functions should return ImcError_t and modify the Pixmap_t pointer */

Rgb_t       imc_alpha_blend(const Rgb_t rgb, const float alpha, const Rgb_t bg_col);
Rgba_t      imc_sample_pixmap(const Pixmap_t pixmap, const float x, const float y);
ImcError_t  imc_scale_pixmap(Pixmap_t *pixmap, const size_t width, const size_t height, const ScaleMethod_t sm);
Pixmap_t    imc_make_pixmap_grayscale(const Pixmap_t pixmap);
Pixmap_t    imc_make_pixmap_ascii_art(Pixmap_t *pixmap);
Pixmap_t    imc_make_pixmap_monochrome(const Pixmap_t pixmap, const float luma_threshold);
void        imc_pixmap_to_ppm(const Pixmap_t pixmap, const char* const fname);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIXMAP_H */
