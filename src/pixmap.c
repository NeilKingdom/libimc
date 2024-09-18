/**
 * @file pixmap.c
 * @author Neil Kingdom
 * @since 17-09-2024
 * @version 1.0
 * @brief Provides public-facing APIs for interacting with Pixmap_t objects.
 */

#include "pixmap.h"

/*
 * ===============================
 *       Private Functions
 * ===============================
 */

static ImcError_t _imc_pixmap_downscale_width(
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
    /* TODO: Do I include bitdepth here? */
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

static ImcError_t _imc_pixmap_upscale_width(
    Pixmap_t *pixmap,
    const size_t width,
    const ScaleMethod_t sm
) {
    return IMC_EOK;
}

static ImcError_t _imc_pixmap_downscale_height(
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

static ImcError_t _imc_pixmap_upscale_height(
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
 * @brief Blends __fg_col__ with __bg_col__ accounting for the opacity given by __alpha__.
 * @since 15-01-2024
 * @param[in] fg_col The foreground pixel's color
 * @param[in] bg_col The background pixel's color
 * @param[in] alpha The opacity/alpha value that will be factored when blending
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

/**
 * @brief Scales an image contained in pixmap to the new size specified by "width" and "height".
 * @since 21-07-2024
 * @param pixmap The pixmap that shall be resized to match "width" and "height"
 * @param width The new width that pixmap shall be scaled to
 * @param height The new height that pixmap shall be scaled to
 * @param sm The scaling method (only used when upscaling)
 * @returns An ImcError_t representing the exit status code
 */
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

/**
 * @brief Converts the image stored in pixmap to grayscale.
 * @since 21-07-2024
 * @param pixmap The pixmap that shall be converted to grayscale
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_grayscale(Pixmap_t *pixmap) {
    int x, y, a;
    size_t scanline_len;

    Pixmap_t tmp;
    Rgb_t rgb;
    Rgba_t rgba;

    /* Weights used to calculate alpha */
    const float r_weight = 0.3f;
    const float g_weight = 0.59f;
    const float b_weight = 0.11f;

    tmp = *pixmap;
    tmp.offset = 0;
    if (pixmap->n_channels < 4) {
        tmp.width /= tmp.n_channels;
        tmp.n_channels = 4;
        tmp.width *= tmp.n_channels;
        /* TODO: Do I include bitdepth here? */
        tmp.data = malloc(tmp.width * tmp.height);
        if (tmp.data == NULL) {
            IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
            return IMC_EFAULT;
        }
    }

    scanline_len = pixmap->width / pixmap->n_channels;
    for (y = 0; y < tmp.height; ++y) {
        for (x = 0; x < scanline_len; ++x) {
            if (pixmap->n_channels == 3) {
                rgb = ((Rgb_t*)pixmap->data)[(y * scanline_len) + x];
                rgba = (Rgba_t){ rgb.r, rgb.g, rgb.b, 255 };
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * scanline_len) + x];
            }

            a = 255 - ((r_weight * rgba.r) + (g_weight * rgba.g) + (b_weight * rgba.b));
            rgba = (Rgba_t){ 0.0f, 0.0f, 0.0f, a };
            ((Rgba_t*)tmp.data)[tmp.offset++] = rgba;
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

/**
 * @brief Transforms the image stored in pixmap to be monochrome according the the value of "luma_threshold".
 * @since 21-07-2024
 * @param pixmap The pixmap to be converted to monochrome
 * @param luma_threshold A normalized threshold between 0.0f-1.0f representing the cutoff between black and white
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_monochrome(Pixmap_t *pixmap, const float luma_threshold) {
    return IMC_EOK;
}

/**
 * @brief Outputs pixmap image as ASCII art to the file specified by "fname".
 * In order to use this function, it is advised that you first make your font as small as possible.
 * To get the number of columns/rows of your terminal, use tput lines and tput cols. Run cat <fname>
 * to print the contents to the TTY.
 * @warning Results will vary depending upon whether you're using RGB or RGBA. This is because RGB uses the
 * color channels to determine luma, whereas, RGBA uses the alpha channel. RGBA cannot use the color channels
 * since that would make this function incompatible with imc_pixmap_to_grayscale() or
 * imc_pixmap_to_monochrome(), which strips the color data.
 * @since 21-07-2024
 * @param pixmap A Pixmap_t containing the image that shall be converted into ASCII art
 * @param fname The name of the file that the ASCII art shall be output to
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_ascii(Pixmap_t *pixmap, const char* const fname) {
    int x, y, luma_idx;
    size_t scanline_len;
    float luma, r_norm, g_norm, b_norm;

    Rgb_t rgb;
    Rgba_t rgba;
    Pixmap_t tmp;
    FILE *fp = NULL;

    /* Weights used to calculate perceived luma */
    const float c = 0.193f;
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
                r_norm = (float)rgb.r / 255.0f;
                g_norm = (float)rgb.g / 255.0f;
                b_norm = (float)rgb.b / 255.0f;

                luma = (r_weight * r_norm) + (g_weight * g_norm) + (b_weight * b_norm);
                luma_idx = roundf(luma * 10) - 1;
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * scanline_len) + x];

                luma = ((float)rgba.a / 255.0f) + c;
                luma_idx = 10 - (roundf(luma * 10) - 1);
            }

            tmp.data[(y * scanline_len) + x] = ascii_luma[(int)imc_clamp(0, 9, luma_idx)];
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

/**
 * @brief Writes the image data in pixmap in PPM format (PPM does not support the alpha channel).
 * @since 15-01-2024
 * @param pixmap A pixmap_t struct containing the data to be written to the PPM file
 * @param fname The name of the output file
 * @param bg_col The background color to be blended in the case that pixbuf contains alpha data
 * @returns An ImcError_t representing the exit status code
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

ImcError_t imc_pixmap_rotate_cw(Pixmap_t *pixmap) {
    Rgb_t rgb;
    Rgba_t rgba;
    Pixmap_t tmp;
    size_t x1, x2, y1, y2, scanline_len1, scanline_len2;

    tmp = *pixmap;
    tmp.width = pixmap->height * pixmap->n_channels;
    tmp.height = pixmap->width / pixmap->n_channels;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    scanline_len1 = tmp.width / tmp.n_channels;
    scanline_len2 = pixmap->width / pixmap->n_channels;
    for (y2 = 0; y2 < pixmap->height; ++y2) {
        for (x2 = 0; x2 < scanline_len2; ++x2) {
            x1 = -y2;
            y1 = x2;

            if ((y1 * scanline_len1) + x1 >= (scanline_len1 * tmp.height)) {
                continue;
            }

            if (tmp.n_channels == 3) {
                ((Rgb_t*)tmp.data)[(y1 * scanline_len1) + x1]
                    = ((Rgb_t*)pixmap->data)[(y2 * scanline_len2) + x2];
            } else if (tmp.n_channels == 4) {
                ((Rgba_t*)tmp.data)[(y1 * scanline_len1) + x1]
                    = ((Rgba_t*)pixmap->data)[(y2 * scanline_len2) + x2];
            }
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

ImcError_t imc_pixmap_rotate_ccw(Pixmap_t *pixmap) {
    Rgb_t rgb;
    Rgba_t rgba;
    Pixmap_t tmp;
    size_t x1, x2, y1, y2, scanline_len1, scanline_len2;

    tmp = *pixmap;
    tmp.width = pixmap->height * pixmap->n_channels;
    tmp.height = pixmap->width / pixmap->n_channels;
    tmp.data = malloc(tmp.width * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    scanline_len1 = tmp.width / tmp.n_channels;
    scanline_len2 = pixmap->width / pixmap->n_channels;
    for (y2 = 0; y2 < pixmap->height; ++y2) {
        for (x2 = 0; x2 < scanline_len2; ++x2) {
            x1 = y2;
            y1 = -x2 + scanline_len2;

            if ((y1 * scanline_len1) + x1 >= (scanline_len1 * tmp.height)) {
                continue;
            }

            if (tmp.n_channels == 3) {
                ((Rgb_t*)tmp.data)[(y1 * scanline_len1) + x1]
                    = ((Rgb_t*)pixmap->data)[(y2 * scanline_len2) + x2];
            } else if (tmp.n_channels == 4) {
                ((Rgba_t*)tmp.data)[(y1 * scanline_len1) + x1]
                    = ((Rgba_t*)pixmap->data)[(y2 * scanline_len2) + x2];
            }
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}
