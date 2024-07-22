#include "pixmap.h"

/*
 * ===============================
 *       Private Functions
 * ===============================
 */

/* TODO: Currently assuming rgba8888, but need to be agnostic to bitdepth and n channels */
ImcError_t _imc_scale_pixmap_width_down(
    Pixmap_t *pixmap,
    const size_t width
) {
    int x, y;
    float tx, ty;
    Rgba_t rgba;
    Pixmap_t tmp = { 0 };

    tmp.bit_depth = pixmap->bit_depth;
    tmp.n_channels = pixmap->n_channels;
    tmp.width = width * sizeof(Rgb_t);
    tmp.height = pixmap->height;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap", IMC_ERROR);
        return IMC_EFAULT;
    }

    for (y = 0; y < tmp.height; ++y) {
        /* TODO: Rather than sizeof(Rgb_t) use pixmap->n_channels * pixmap->bit_depth */
        for (x = 0; x < tmp.width; x += sizeof(Rgb_t)) {
            tx = (float)x / (float)tmp.width;
            ty = (float)y / (float)tmp.height;

            rgba = imc_sample_pixmap(*pixmap, tx, ty);
            tmp.data[(y * tmp.width) + x + 0] = rgba.r;
            tmp.data[(y * tmp.width) + x + 1] = rgba.g;
            tmp.data[(y * tmp.width) + x + 2] = rgba.b;
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

void _imc_scale_pixmap_width_up(
    const Pixmap_t *pixmap,
    const size_t width,
    const ScaleMethod_t sm
) {

}

ImcError_t _imc_scale_pixmap_height_down(
    Pixmap_t *pixmap,
    const size_t height
) {
    int x, y;
    float tx, ty;
    Rgba_t rgba;
    Pixmap_t tmp = { 0 };

    tmp.bit_depth = pixmap->bit_depth;
    tmp.n_channels = pixmap->n_channels;
    tmp.width = pixmap->width;
    tmp.height = height;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap", IMC_ERROR);
        return IMC_EFAULT;
    }

    for (y = 0; y < tmp.height; ++y) {
        for (x = 0; x < tmp.width; x += sizeof(Rgb_t)) {
            tx = (float)x / (float)tmp.width;
            ty = (float)y / (float)tmp.height;

            rgba = imc_sample_pixmap(*pixmap, tx, ty);
            tmp.data[(y * tmp.width) + x + 0] = rgba.r;
            tmp.data[(y * tmp.width) + x + 1] = rgba.g;
            tmp.data[(y * tmp.width) + x + 2] = rgba.b;
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

void _imc_scale_pixmap_height_up(
    const Pixmap_t *pixmap,
    const size_t height,
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
    float a = alpha / 255.0f;
    r = (1.0f - a) * bg_col.r + a * rgb.r;
    g = (1.0f - a) * bg_col.g + a * rgb.g;
    b = (1.0f - a) * bg_col.b + a * rgb.b;

    return (Rgb_t){ r, g, b };
}

/**
 * @brief Returns the RGB value of the pixel specified by the normalized x and y values
 * @warning If x or y lie outside of the specified bounds (0.0f-1.0f) they will be clamped
 * @since 15-01-2024
 * @param pixmap The pixmap that will be sampled
 * @param x A normalized floating point value between 0.0f and 1.0f representing the x sampling component
 * @param y A normalized floating point value between 0.0f and 1.0f representing the y sampling component
 * @returns An Rgba_t struct containing the sampled pixel color value
 */
Rgba_t imc_sample_pixmap(const Pixmap_t pixmap, const float x, const float y) {
    Rgba_t rgba = { 0 };
    int x_intrp, y_intrp;
    float _x, _y;

    _x = imc_clamp(0.0f, 1.0f, x);
    if (_x != x) {
        IMC_LOG(
            "The value provided for x was outside the acceptable range (0.0f-1.0f) and will be clamped",
            IMC_WARNING
        );
    }
    _y = imc_clamp(0.0f, 1.0f, y);
    if (_y != y) {
        IMC_LOG(
            "The value provided for y was outside the acceptable range (0.0f-1.0f) and will be clamped",
            IMC_WARNING
        );
    }

    y_intrp = pixmap.width * (int)roundf(y * pixmap.height);
    x_intrp = imc_lerp(1.0f, pixmap.width, _x);
    /* Make sure we're aligned on the start of a pixel boundary */
    x_intrp -= x_intrp % pixmap.n_channels;

    rgba.r = pixmap.data[y_intrp + x_intrp + 0];
    rgba.g = pixmap.data[y_intrp + x_intrp + 1];
    rgba.b = pixmap.data[y_intrp + x_intrp + 2];

    if (pixmap.n_channels == 4) {
        rgba.a = pixmap.data[y_intrp + x_intrp + 3];
    } else {
        rgba.a = 255;
    }

    return rgba;
}

ImcError_t imc_scale_pixmap(
    Pixmap_t *pixmap,
    const size_t width,
    const size_t height,
    const ScaleMethod_t sm
) {
    if (width < pixmap->width) {
        _imc_scale_pixmap_width_down(pixmap, width);
    } else if (width > pixmap->width) {
        _imc_scale_pixmap_width_up(pixmap, width, sm);
    }

    if (height < pixmap->height) {
        _imc_scale_pixmap_height_down(pixmap, height);
    } else if (height > pixmap->height) {
        _imc_scale_pixmap_height_up(pixmap, height, sm);
    }

    return IMC_EOK;
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
 * To get the number of columns/rows of your terminal, use tput lines and tput cols.
 * @since 21-07-2024
 * @param pixmap A Pixmap_t containing the image that shall be converted into ASCII art
 * @param ncols The number of columns that your terminal is capable of outputting
 * @param nrows The number of rows that your terminal is capable of outputting
 * @returns A Pixmap_t containing ASCII characters in the data buffer rather than rgb values
 */
Pixmap_t imc_make_pixmap_ascii_art(Pixmap_t *pixmap) {
    Rgb_t rgb;
    Pixmap_t tmp = *pixmap;
    int x, y, luma_idx;
    float luma, r_norm, g_norm, b_norm;
    const float r_weight = 0.2126f;
    const float g_weight = 0.7152f;
    const float b_weight = 0.0722f;
    const char ascii_luma[10] = { ' ', '.', ':', '-', '=', '+', '*', '#', '%', '@' };

    tmp.data = malloc(tmp.width * tmp.height);

    for (y = 0; y < pixmap->height; ++y) {
        /* TODO: Rather than sizeof(Rgb_t) use pixmap->n_channels * pixmap->bit_depth */
        for (x = 0; x < pixmap->width; x += sizeof(Rgb_t)) {
            rgb.r = pixmap->data[(y * pixmap->width) + x + 0];
            rgb.g = pixmap->data[(y * pixmap->width) + x + 1];
            rgb.b = pixmap->data[(y * pixmap->width) + x + 2];

            r_norm = (float)rgb.r / 255.0f;
            g_norm = (float)rgb.g / 255.0f;
            b_norm = (float)rgb.b / 255.0f;

            luma = (r_weight * r_norm) + (g_weight * g_norm) + (b_weight * b_norm);
            luma_idx = roundf(luma * 10) - 1;
            luma_idx = imc_clamp(0, 9, luma_idx);
            tmp.data[(y * pixmap->width) + (x / sizeof(Rgb_t))] = ascii_luma[luma_idx];
        }
    }

    free(pixmap->data);
    pixmap->data = tmp.data;

    return (Pixmap_t){ 0 };
}

Pixmap_t imc_make_pixmap_monochrome(const Pixmap_t pixmap, const float luma_threshold) {
    return (Pixmap_t){ 0 };
}

/**
 * @brief Outputs a PPM file containing the pixmap's data.
 * @since 15-01-2024
 * @param fname The name of the output file
 * @param pixmap A pixmap_t struct containing the data to be written to the PPM file
 */
void imc_pixmap_to_ppm(const Pixmap_t pixmap, const char* const fname) {
    Rgb_t bg_col = (Rgb_t){ 255, 255, 255 };
    size_t x, y, bpp, scanline_len;
    FILE *fp = NULL;

    bpp = (size_t)powf(2.0f, pixmap.bit_depth) - 1.0f;
    scanline_len = pixmap.width / pixmap.n_channels;
    fp = fopen(fname, "wb");

    /* Header */
    fputs("P6\n", fp);
    fprintf(fp, "%ld %ld\n", scanline_len, pixmap.height);
    fprintf(fp, "%ld\n", bpp);

    /* Data */
    for (y = 0; y < pixmap.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            if (pixmap.n_channels == 3) {
                Rgb_t rgb = ((Rgb_t*)pixmap.data)[(y * scanline_len) + x];
                fputc(rgb.r, fp);
                fputc(rgb.g, fp);
                fputc(rgb.b, fp);
            } else if (pixmap.n_channels == 4) {
                Rgba_t rgba = ((Rgba_t*)pixmap.data)[(y * scanline_len) + x];
                Rgb_t rgb = imc_alpha_blend((Rgb_t){ rgba.r, rgba.g, rgba.b }, rgba.a, bg_col);
                fputc(rgb.r, fp);
                fputc(rgb.g, fp);
                fputc(rgb.b, fp);
            }
        }
    }

    fclose(fp);
}

