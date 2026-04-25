#ifndef PIXMAP_H
#define PIXMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "imc_common.h"
#include "imc_utils.h"

typedef struct {
    usize       width;      // Width of the image (in pixels)
    usize       height;     // Height of the image (in pixels)
    usize       offset;     // Current read offset into the data buffer (in bytes)
    PixFormat_t n_channels; // Number of color channels
    u8          bit_depth;  // Number of bits per-channel
    u8         *data;       // Raw data of the pixmap
} Pixmap_t;

typedef enum {
    NEAREST,  // Nearest-neighbor
    BILINEAR, // Bilinear interpolation
    BICUBIC   // Bicubic interpolation
} ScaleMethod_t;

// Forward function declarations

usize       imc_sizeof_px(const Pixmap_t pixmap);
Rgba_t      imc_pixmap_nsample(Pixmap_t *pixmap, const f32 x, const f32 y);
Rgba_t      imc_pixmap_psample(Pixmap_t *pixmap, const usize x, const usize y);
ImcResult_t imc_pixmap_scale(Pixmap_t *pixmap, const usize width, const usize height, const ScaleMethod_t sm);
ImcResult_t imc_pixmap_to_grayscale(Pixmap_t *pixmap);
ImcResult_t imc_pixmap_to_monochrome(Pixmap_t *pixmap, const f32 luma_threshold);
ImcResult_t imc_pixmap_to_ascii(Pixmap_t *pixmap, const char* const fname);
ImcResult_t imc_pixmap_to_ppm(Pixmap_t *pixmap, const char* const fname, const Rgb_t bg_col);
ImcResult_t imc_pixmap_rotate_cw(Pixmap_t *pixmap);
ImcResult_t imc_pixmap_rotate_ccw(Pixmap_t *pixmap);
ImcResult_t imc_pixmap_destroy(Pixmap_t *pixmap);

#ifdef __cplusplus
}
#endif

#endif
