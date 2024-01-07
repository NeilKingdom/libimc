#ifndef PIX_BUF_H
#define PIX_BUF_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct PixBuf {
    pixel_t *data;
    size_t   size;
} pix_map_t;

typedef enum {
    NEAREST,    /* Nearest-neighbor */
    BILINEAR,   /* Bilinear interpolation */
} ScaleMethod;

pix_map_t   imc_scale_pixbuf(const pix_map_t pb, const ScaleMethod sm, const float x_tgt, const float y_tgt);
pixel_t     imc_get_pixbuf_pixel(const pix_map_t pb, const float x, const float y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PIX_BUF_H */
