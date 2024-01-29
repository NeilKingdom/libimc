#include "../include/pixmap.h"

rgba_t      imc_sample_pixbuf(const pixmap_t pixmap, const float x, const float y) {
    rgba_t rgba = { 0 };
    rgba.r = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 0)];
    rgba.g = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 1)];
    rgba.b = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 2)];
    rgba.a = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 3)];
    return rgba;
}

pixmap_t    imc_scale_pixbuf(const pixmap_t pixmap, const float width, const float height, const ScaleMethod sm) {
    return (pixmap_t){ 0 }; 
}

/* TODO: Currently assuming rgba8888, but need to be agnostic to bitdepth and n channels */
pixmap_t    imc_make_pixbuf_grayscale(const pixmap_t pixmap) {
    int x, y;
    size_t scanline_len;
    const float r_weight = 0.3f;
    const float g_weight = 0.59f;
    const float b_weight = 0.11f;
    pixmap_t out = { 0 };

    scanline_len = pixmap.width / 4; /* 4 is n_channels */
    out.width = pixmap.width;
    out.height = pixmap.height;
    out.data = malloc(out.width * out.height);
    if (out.data == NULL) {
        IMC_WARN("Failed to allocate memory for the pixmap's data");
        return out;
    }

    memcpy(out.data, pixmap.data, out.width * out.height);
    for (y = 0; y < out.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            rgba_t *rgba = &((rgba_t*)out.data)[(y * scanline_len) + x];
            rgba->a = 255 - ((r_weight * rgba->r) + (g_weight * rgba->g) + (b_weight * rgba->b));
            rgba->r = 0.0f;
            rgba->g = 0.0f;
            rgba->b = 0.0f;
        }
    } 

    return out;
}

pixmap_t    imc_make_pixbuf_monochrome(const pixmap_t pixmap, const float luma_threshold) {
    return (pixmap_t){ 0 }; 
}
