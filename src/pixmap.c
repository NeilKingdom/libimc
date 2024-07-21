#include "pixmap.h"

/*
 * ===============================
 *       Private Functions
 * ===============================
 */

void _imc_scale_pixmap_width_down(
    const Pixmap_t *pixmap,
    const float width
) {

}

void _imc_scale_pixmap_width_up(
    const Pixmap_t *pixmap,
    const float width,
    const ScaleMethod_t sm
) {

}

void _imc_scale_pixmap_height_down(
    const Pixmap_t *pixmap,
    const float height
) {

}

void _imc_scale_pixmap_height_up(
    const Pixmap_t *pixmap,
    const float height,
    const ScaleMethod_t sm
) {

}

/*
 * ===============================
 *       Public Functions
 * ===============================
 */

/**
 * @brief Blends "rgb" with "bg_col" accounting for alpha.
 * @since 15-01-2024
 * @param rgb The foreground pixel's color
 * @param alpha The transparancy/alpha value that will be factored when blending
 * @param bg_col The background pixel's color
 * @returns A new Rgb_t containing the blended rgb pixel value
 */
Rgb_t imc_alpha_blend(const Rgb_t rgb, const float alpha, const Rgb_t bg_col) {
    uint8_t r, g, b;
    float _alpha = alpha / 255.0f;
    r = (1.0f - _alpha) * bg_col.r + _alpha * rgb.r;
    g = (1.0f - _alpha) * bg_col.g + _alpha * rgb.g;
    b = (1.0f - _alpha) * bg_col.b + _alpha * rgb.b;
    return (Rgb_t){ r, g, b };
}

Rgba_t imc_sample_pixmap(const Pixmap_t pixmap, const float x, const float y) {
    Rgba_t rgba = { 0 };
    rgba.r = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 0)];
    rgba.g = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 1)];
    rgba.b = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 2)];
    rgba.a = pixmap.data[(int)((pixmap.width * roundf(y)) + roundf(x) + 3)];
    return rgba;
}

Pixmap_t imc_scale_pixmap(
    const Pixmap_t pixmap,
    const float width,
    const float height,
    const ScaleMethod_t sm
) {
    Pixmap_t tmp = pixmap;

    /* Scale down first because it's slightly more efficient */
    if (width < pixmap.width) {
        _imc_scale_pixmap_width_down(&tmp, width);
    }
    if (height < pixmap.height) {
        _imc_scale_pixmap_height_down(&tmp, height);
    }
    if (width > pixmap.width) {
        _imc_scale_pixmap_width_up(&tmp, width, sm);
    }
    if (height > pixmap.height) {
        _imc_scale_pixmap_height_up(&tmp, height, sm);
    }

    return tmp;
}

/* TODO: Currently assuming rgba8888, but need to be agnostic to bitdepth and n channels */
Pixmap_t imc_make_pixmap_grayscale(const Pixmap_t pixmap) {
    int x, y;
    size_t scanline_len;
    const float r_weight = 0.3f;
    const float g_weight = 0.59f;
    const float b_weight = 0.11f;
    Pixmap_t out = { 0 };

    scanline_len = pixmap.width / 4; /* 4 is n_channels */
    out.width = pixmap.width;
    out.height = pixmap.height;
    out.data = malloc(out.width * out.height);
    if (out.data == NULL) {
        IMC_LOG("Failed to allocate memory for the pixmap's data", IMC_ERROR);
        return out;
    }

    memcpy(out.data, pixmap.data, out.width * out.height);
    for (y = 0; y < out.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            Rgba_t *rgba = &((Rgba_t*)out.data)[(y * scanline_len) + x];
            rgba->a = 255 - ((r_weight * rgba->r) + (g_weight * rgba->g) + (b_weight * rgba->b));
            rgba->r = 0.0f;
            rgba->g = 0.0f;
            rgba->b = 0.0f;
        }
    }

    return out;
}

/**
 * @brief Turns an image into ASCII art.
 * In order to use this function, it is advised that you first make your font as small as possible.
 * To get the number of columns/rows of your terminal, use tput rows and tput cols.
 * @since 21-07-2024
 * @param pixmap A Pixmap_t containing the image that shall be converted into ASCII art
 * @param ncols The number of columns that your terminal is capable of outputting
 * @param nrows The number of rows that your terminal is capable of outputting
 * @returns A Pixmap_t containing ASCII characters in the data buffer rather than rgb values
 */
Pixmap_t imc_make_pixmap_ascii_art(const Pixmap_t pixmap, const size_t ncols, const size_t nrows) {
    return (Pixmap_t){ 0 };
}

Pixmap_t imc_make_pixmap_monochrome(const Pixmap_t pixmap, const float luma_threshold) {
    return (Pixmap_t){ 0 };
}
