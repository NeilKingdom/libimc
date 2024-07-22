#include "pixmap.h"

/*
 * ===============================
 *       Private Functions
 * ===============================
 */

ImcError_t _imc_pixmap_downscale_width(
    Pixmap_t *pixmap,
    const size_t width
) {
    int x, y;
    float tx, ty;
    size_t scanline_len;

    Pixmap_t tmp;
    Rgba_t rgba;
    Rgba_t *p_rgba = NULL;
    Rgb_t *p_rgb = NULL;

    tmp = *pixmap;
    tmp.width = width * pixmap->n_channels;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap", IMC_ERROR);
        return IMC_EFAULT;
    }

    scanline_len = tmp.width / tmp.n_channels;

    for (y = 0; y < tmp.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            tx = (float)x / (float)scanline_len;
            ty = (float)y / (float)tmp.height;

            rgba = imc_pixmap_sample(pixmap, tx, ty);
            if (pixmap->n_channels == 3) {
                p_rgb = &((Rgb_t*)tmp.data)[(y * scanline_len) + x];
                *p_rgb = (Rgb_t){ rgba.r, rgba.g, rgba.b };
            } else if (pixmap->n_channels == 4) {
                p_rgba = &((Rgba_t*)tmp.data)[(y * scanline_len) + x];
                *p_rgba = rgba;
            }
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

ImcError_t _imc_pixmap_upscale_width(
    Pixmap_t *pixmap,
    const size_t width,
    const ScaleMethod_t sm
) {
    return IMC_EOK;
}

ImcError_t _imc_pixmap_downscale_height(
    Pixmap_t *pixmap,
    const size_t height
) {
    int x, y;
    float tx, ty;
    size_t scanline_len;

    Pixmap_t tmp;
    Rgba_t rgba;
    Rgba_t *p_rgba = NULL;
    Rgb_t *p_rgb = NULL;

    tmp = *pixmap;
    tmp.height = height;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap", IMC_ERROR);
        return IMC_EFAULT;
    }

    scanline_len = tmp.width / tmp.n_channels;

    for (y = 0; y < tmp.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            tx = (float)x / (float)scanline_len;
            ty = (float)y / (float)tmp.height;

            rgba = imc_pixmap_sample(pixmap, tx, ty);
            if (pixmap->n_channels == 3) {
                p_rgb = &((Rgb_t*)tmp.data)[(y * scanline_len) + x];
                *p_rgb = (Rgb_t){ rgba.r, rgba.g, rgba.b };
            } else if (pixmap->n_channels == 4) {
                p_rgba = &((Rgba_t*)tmp.data)[(y * scanline_len) + x];
                *p_rgba = rgba;
            }
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

ImcError_t _imc_pixmap_upscale_height(
    Pixmap_t *pixmap,
    const size_t height,
    const ScaleMethod_t sm
) {
    return IMC_EOK;
}

/*
 * ===============================
 *       Public Functions
 * ===============================
 */

/* TODO: Do we need to clamp alpha? */
/**
 * @brief Blends "fg_col" with "bg_col" accounting for the opacity given by "alpha".
 * @since 15-01-2024
 * @param fg_col The foreground pixel's color
 * @param bg_col The background pixel's color
 * @param alpha The opacity/alpha value that will be factored when blending
 * @returns A new Rgb_t containing the blended rgb pixel value
 */
Rgb_t imc_blend_alpha(const Rgb_t fg_col, const Rgb_t bg_col, const float alpha) {
    uint8_t r, g, b;
    float a = alpha / 255.0f;
    r = (1.0f - a) * bg_col.r + a * fg_col.r;
    g = (1.0f - a) * bg_col.g + a * fg_col.g;
    b = (1.0f - a) * bg_col.b + a * fg_col.b;

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
Rgba_t imc_pixmap_sample(Pixmap_t *pixmap, const float x, const float y) {
    float _x, _y;
    int x_intrp, y_intrp;
    size_t scanline_len;

    Rgb_t rgb;
    Rgba_t rgba;

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

    scanline_len = pixmap->width / pixmap->n_channels;

    y_intrp = scanline_len * (int)roundf(y * pixmap->height);
    x_intrp = imc_lerp(1.0f, pixmap->width, _x);
    /* Make sure we're aligned on the start of a pixel boundary */
    x_intrp -= x_intrp % pixmap->n_channels;
    /* Can't use scanline_len for lerp, so need to scale down afterwards */
    x_intrp = x_intrp / pixmap->n_channels;

    if (pixmap->n_channels == 3) {
        rgb = ((Rgb_t*)pixmap->data)[y_intrp + x_intrp];
        rgba = (Rgba_t){ rgb.r, rgb.g, rgb.b, 255 };
    } else if (pixmap->n_channels == 4) {
        rgba = ((Rgba_t*)pixmap->data)[y_intrp + x_intrp];
    }

    return rgba;
}

ImcError_t imc_pixmap_scale(
    Pixmap_t *pixmap,
    const size_t width,
    const size_t height,
    const ScaleMethod_t sm
) {
    int status;

    if (width < pixmap->width) {
        status = _imc_pixmap_downscale_width(pixmap, width);
        if (status != IMC_EOK) {
            return IMC_EFAIL;
        }
    } else if (width > pixmap->width) {
        status = _imc_pixmap_upscale_width(pixmap, width, sm);
        if (status != IMC_EOK) {
            return IMC_EFAIL;
        }
    }

    if (height < pixmap->height) {
        status = _imc_pixmap_downscale_height(pixmap, height);
        if (status != IMC_EOK) {
            return IMC_EFAIL;
        }
    } else if (height > pixmap->height) {
        status = _imc_pixmap_upscale_height(pixmap, height, sm);
        if (status != IMC_EOK) {
            return IMC_EFAIL;
        }
    }

    return IMC_EOK;
}

/* TODO: Can we make this work for RGB888 as well? */
ImcError_t imc_pixmap_to_grayscale(Pixmap_t *pixmap) {
    int x, y;
    size_t scanline_len;

    const float r_weight = 0.3f;
    const float g_weight = 0.59f;
    const float b_weight = 0.11f;

    scanline_len = pixmap->width / pixmap->n_channels;

    for (y = 0; y < pixmap->height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            Rgba_t *rgba = &((Rgba_t*)pixmap->data)[(y * scanline_len) + x];
            rgba->r = 0.0f;
            rgba->g = 0.0f;
            rgba->b = 0.0f;
            rgba->a = 255 - ((r_weight * rgba->r) + (g_weight * rgba->g) + (b_weight * rgba->b));
        }
    }

    return IMC_EOK;
}

/**
 * @brief Outputs pixmap image as ASCII art to the file specified by "fname".
 * In order to use this function, it is advised that you first make your font as small as possible.
 * To get the number of columns/rows of your terminal, use tput lines and tput cols. Run cat <file>
 * to print the contents to the TTY.
 * @since 21-07-2024
 * @param pixmap A Pixmap_t containing the image that shall be converted into ASCII art
 * @param fname The name of the file that the ASCII art shall be output to
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_ascii(Pixmap_t *pixmap, const char* const fname) {
    int x, y, luma_idx;
    float luma, r_norm, g_norm, b_norm;
    size_t scanline_len;

    Rgb_t rgb;
    Rgba_t rgba;
    Pixmap_t tmp;
    FILE *fp = NULL;

    /* Weights used to calculate perceived luma */
    const float r_weight = 0.2126f;
    const float g_weight = 0.7152f;
    const float b_weight = 0.0722f;
    const char ascii_luma[10] = { ' ', '.', ':', '-', '=', '+', '*', '#', '%', '@' };

    tmp = *pixmap;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    scanline_len = pixmap->width / pixmap->n_channels;

    for (y = 0; y < pixmap->height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            if (pixmap->n_channels == 3) {
                rgb = ((Rgb_t*)pixmap->data)[(y * scanline_len) + x];
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * scanline_len) + x];
                rgb = (Rgb_t){ rgba.r, rgba.g, rgba.b };
            }

            r_norm = (float)rgb.r / 255.0f;
            g_norm = (float)rgb.g / 255.0f;
            b_norm = (float)rgb.b / 255.0f;

            luma = (r_weight * r_norm) + (g_weight * g_norm) + (b_weight * b_norm);
            luma_idx = roundf(luma * 10) - 1;
            luma_idx = imc_clamp(0, 9, luma_idx);

            tmp.data[(y * scanline_len) + x] = ascii_luma[luma_idx];
        }
    }

    free(pixmap->data);
    pixmap->data = tmp.data;

    /* Output to file */
    fp = fopen(fname, "wb");
    if (fp == NULL) {
        IMC_LOG("Failed to open file for write", IMC_ERROR);
        return IMC_EFAIL;
    }

    for (y = 0; y < pixmap->height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            putc(pixmap->data[(y * scanline_len) + x], fp);
        }
        putc('\n', fp);
    }

    return IMC_EOK;
}

ImcError_t imc_pixmap_to_monochrome(Pixmap_t *pixmap, const float luma_threshold) {
    return IMC_EOK;
}

/**
 * @brief Outputs a PPM file containing the pixmap's data.
 * @since 15-01-2024
 * @param fname The name of the output file
 * @param pixmap A pixmap_t struct containing the data to be written to the PPM file
 */
ImcError_t imc_pixmap_to_ppm(Pixmap_t *pixmap, const char* const fname, const Rgb_t bg_col) {
    Rgb_t rgb;
    Rgba_t rgba;
    size_t x, y, bpp, scanline_len;
    FILE *fp = NULL;

    fp = fopen(fname, "wb");
    if (fp == NULL) {
        IMC_LOG("Failed to open file for write", IMC_ERROR);
        return IMC_EFAIL;
    }

    bpp = (size_t)powf(2.0f, pixmap->bit_depth) - 1.0f;
    scanline_len = pixmap->width / pixmap->n_channels;

    /* Write PPM header */
    fputs("P6\n", fp);
    fprintf(fp, "%ld %ld\n", scanline_len, pixmap->height);
    fprintf(fp, "%ld\n", bpp);

    /* Write PPM data */
    for (y = 0; y < pixmap->height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            if (pixmap->n_channels == 3) {
                rgb = ((Rgb_t*)pixmap->data)[(y * scanline_len) + x];
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * scanline_len) + x];
                rgb = imc_blend_alpha((Rgb_t){ rgba.r, rgba.g, rgba.b }, bg_col, rgba.a);
            }

            fputc(rgb.r, fp);
            fputc(rgb.g, fp);
            fputc(rgb.b, fp);
        }
    }

    fclose(fp);
    return IMC_EOK;
}

