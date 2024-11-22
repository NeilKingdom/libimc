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

// TODO: Implement scaling functions

static ImcError_t _imc_pixmap_downscale_width(
    Pixmap_t *pixmap,
    const size_t width,
    const ScaleMethod_t sm
) {
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
    const size_t height,
    const ScaleMethod_t sm
) {
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

/**
 * @brief Returns the size in bytes of a single pixel within __pixmap__.
 * @since 15-10-2024
 * @param[in] pixmap The input pixmap who's pixel size is being determined
 * @returns The size in bytes of a single pixel within __pixmap__
 */
inline size_t imc_sizeof_px(const Pixmap_t pixmap) {
    /* Practically speaking, only a bit-depth of 16 will occupy 2 bytes, whereas other bit-depths will occupy 1 */
    return (pixmap.bit_depth > 8) ? pixmap.n_channels * 2 : pixmap.n_channels;
}

/**
 * @brief Blends __fg_col__ with __bg_col__ accounting for the opacity given by __alpha__.
 * @since 15-01-2024
 * @param[in] fg_col The foreground color
 * @param[in] bg_col The background color
 * @param[in] alpha The opacity/alpha value that will be factored when blending
 * @returns A new Rgb_t containing the blended rgb pixel value
 */
Rgb_t imc_blend_alpha(const Rgb_t fg_col, const Rgb_t bg_col, const uint8_t alpha) {
    uint8_t r, g, b;
    float a = (float)alpha / 255.0f;
    r = (1.0f - a) * bg_col.r + a * fg_col.r;
    g = (1.0f - a) * bg_col.g + a * fg_col.g;
    b = (1.0f - a) * bg_col.b + a * fg_col.b;

    return (Rgb_t){ r, g, b };
}

/**
 * @brief Returns an RGBA value representing the pixel sampled from the normalized coordinates __x__ and __y__.
 * @warning If __x__ or __y__ lie outside of the specified bounds (0.0f-1.0f) they will be clamped.
 * @since 15-01-2024
 * @param[in] pixmap The pixmap that will be sampled
 * @param[in] x A normalized floating point value between 0.0f and 1.0f representing the x sampling component
 * @param[in] y A normalized floating point value between 0.0f and 1.0f representing the y sampling component
 * @returns An Rgba_t struct representing the color of the sampled pixel
 */
Rgba_t imc_pixmap_nsample(Pixmap_t *pixmap, const float x, const float y) {
    float _x, _y;
    int x_intrp, y_intrp;

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

    y_intrp = pixmap->width * (int)roundf(_y * pixmap->height);
    x_intrp = imc_lerp(0.0f, pixmap->width - 1.0f, _x);
    /* Make sure we're aligned on the start of a pixel boundary */
    x_intrp = x_intrp / imc_sizeof_px(*pixmap);

    if (pixmap->n_channels == 3) {
        rgb = ((Rgb_t*)pixmap->data)[y_intrp + x_intrp];
        rgba = (Rgba_t){ rgb.r, rgb.g, rgb.b, 255 };
    } else if (pixmap->n_channels == 4) {
        rgba = ((Rgba_t*)pixmap->data)[y_intrp + x_intrp];
    }

    return rgba;
}

/**
 * @brief Returns an RGBA value representing the pixel sampled from the coordinates __x__ and __y__ (non-normalized).
 * @warning If __x__ or __y__ lie outside of the width of height of the pixmap, they will be clamped.
 * @since 15-10-2024
 * @param[in] pixmap The pixmap that will be sampled
 * @param[in] x The 0-indexed x sampling component, which represents the horizontal offset into the pixmap
 * @param[in] y The 0-indexed y sampling component, which represents the vertical offset into the pixmap
 * @returns An Rgba_t struct representing the color of the sampled pixel
 */
Rgba_t imc_pixmap_psample(Pixmap_t *pixmap, const size_t x, const size_t y) {
    size_t _x = x, _y = y;

    Rgb_t rgb;
    Rgba_t rgba;

    if (x >= pixmap->width) {
        _x = pixmap->width - 1;
    }

    if (y >= pixmap->height) {
        _y = pixmap->height - 1;
    }

    if (pixmap->n_channels == 3) {
        rgb = ((Rgb_t*)pixmap->data)[(_y * pixmap->width) + _x];
        rgba = (Rgba_t){ rgb.r, rgb.g, rgb.b, 255 };
    } else if (pixmap->n_channels == 4) {
        rgba = ((Rgba_t*)pixmap->data)[(_y * pixmap->width) + _x];
    }

    return rgba;
}

/**
 * @brief Scales an image contained in pixmap to the new size specified by __width__ and __height__.
 * @since 21-07-2024
 * @param[in,out] pixmap The pixmap that shall be resized to match __width__ and __height__
 * @param[in] width The new width that pixmap shall be scaled to
 * @param[in] height The new height that pixmap shall be scaled to
 * @param[in] sm The scaling method (only used when upscaling)
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
        status = _imc_pixmap_downscale_width(pixmap, width, sm);
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
        status = _imc_pixmap_downscale_height(pixmap, height, sm);
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
 * @brief Converts the image stored in __pixmap__ to grayscale.
 * @since 21-07-2024
 * @param[in,out] pixmap The pixmap that shall be converted to grayscale
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_grayscale(Pixmap_t *pixmap) {
    int x, y, a;

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
        tmp.data = malloc(tmp.width * tmp.height * sizeof(Rgba_t));
        if (tmp.data == NULL) {
            IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
            return IMC_EFAULT;
        }
    }

    for (y = 0; y < tmp.height; ++y) {
        for (x = 0; x < pixmap->width; ++x) {
            if (pixmap->n_channels == 3) {
                rgb = ((Rgb_t*)pixmap->data)[(y * pixmap->width) + x];
                rgba = (Rgba_t){ rgb.r, rgb.g, rgb.b, 255 };
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * pixmap->width) + x];
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
 * @brief Transforms the image stored in pixmap to be monochrome according the the value of __luma_threshold__.
 * @since 21-07-2024
 * @param[in,out] pixmap The pixmap to be converted to monochrome
 * @param[in] luma_threshold A normalized threshold between 0.0f-1.0f representing the cutoff between black and white
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_monochrome(Pixmap_t *pixmap, const float luma_threshold) {
    return IMC_EOK;
}

/**
 * @brief Outputs pixmap image as ASCII art to the file specified by __fname__.
 * In order to use this function, it is advised that you first make your font as small as possible.
 * To get the number of columns/rows of your terminal, use tput lines and tput cols and then scale
 * the pixmap before running cat <fname> to echo the contents of the file to the TTY.
 * @warning Results will vary depending upon whether you're using RGB or RGBA. This is because RGB uses the
 * color channels to determine luma, whereas, RGBA uses the alpha channel. RGBA cannot use the color channels
 * since that would make this function incompatible with imc_pixmap_to_grayscale() or
 * imc_pixmap_to_monochrome(), which strips the color data.
 * @since 21-07-2024
 * @param[in,out] pixmap A Pixmap_t containing the image that shall be converted into ASCII art
 * @param[in] fname The name of the file that the ASCII art shall be output to
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_ascii(Pixmap_t *pixmap, const char* const fname) {
    int x, y, luma_idx;
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
    tmp.data = malloc(tmp.width * tmp.height * sizeof(char));
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    for (y = 0; y < pixmap->height; ++y) {
        for (x = 0; x < pixmap->width; ++x) {
            if (pixmap->n_channels == 3) {
                rgb = ((Rgb_t*)pixmap->data)[(y * pixmap->width) + x];
                r_norm = (float)rgb.r / 255.0f;
                g_norm = (float)rgb.g / 255.0f;
                b_norm = (float)rgb.b / 255.0f;

                luma = (r_weight * r_norm) + (g_weight * g_norm) + (b_weight * b_norm);
                luma_idx = roundf(luma * 10) - 1;
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * pixmap->width) + x];

                luma = ((float)rgba.a / 255.0f) + c;
                luma_idx = 10 - (roundf(luma * 10) - 1);
            }

            tmp.data[(y * pixmap->width) + x] = ascii_luma[(int)imc_clamp(0, 9, luma_idx)];
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
        for (x = 0; x < pixmap->width; ++x) {
            putc(pixmap->data[(y * pixmap->width) + x], fp);
        }
        putc('\n', fp);
    }

    return IMC_EOK;
}

/**
 * @brief Writes the image data in pixmap in PPM format (PPM does not support the alpha channel).
 * @since 15-01-2024
 * @param[in] pixmap A Pixmap_t struct containing the data to be written to the PPM file
 * @param[in] fname The name of the output file
 * @param[in] bg_col The background color to be blended in the case that pixbuf contains alpha data
 * @returns An ImcError_t representing the exit status code
 */
ImcError_t imc_pixmap_to_ppm(Pixmap_t *pixmap, const char* const fname, const Rgb_t bg_col) {
    size_t x, y, bpp;

    Rgb_t rgb;
    Rgba_t rgba;
    FILE *fp = NULL;

    fp = fopen(fname, "wb");
    if (fp == NULL) {
        IMC_LOG("Failed to open file for write", IMC_ERROR);
        return IMC_EFAIL;
    }

    bpp = (size_t)powf(2.0f, pixmap->bit_depth) - 1.0f;

    /* Write PPM header */
    fputs("P6\n", fp);
    fprintf(fp, "%ld %ld\n", pixmap->width, pixmap->height);
    fprintf(fp, "%ld\n", bpp);

    /* Write PPM data */
    for (y = 0; y < pixmap->height; ++y) {
        for (x = 0; x < pixmap->width; ++x) {
            if (pixmap->n_channels == 3) {
                rgb = ((Rgb_t*)pixmap->data)[(y * pixmap->width) + x];
            } else if (pixmap->n_channels == 4) {
                rgba = ((Rgba_t*)pixmap->data)[(y * pixmap->width) + x];
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

/**
 * @brief Rotate the image contained in __pixmap__ 90 degrees clockwise.
 * @since 07-09-2024
 * @param[in,out] pixmap The pixmap to be rotated 90 degrees clockwise
 * @returns An ImcError_t representing the exit status code of the function
 */
ImcError_t imc_pixmap_rotate_cw(Pixmap_t *pixmap) {
    size_t x1, x2, y1, y2;

    Pixmap_t tmp;
    Rgb_t rgb;
    Rgba_t rgba;

    tmp = *pixmap;
    tmp.width = pixmap->height;
    tmp.height = pixmap->width;
    tmp.data = malloc((tmp.width * imc_sizeof_px(tmp)) * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    for (y2 = 0; y2 < pixmap->height; ++y2) {
        for (x2 = 0; x2 < pixmap->width; ++x2) {
            x1 = -y2 - 1;
            y1 = x2 + 1;

            if ((y1 * tmp.width) + x1 >= (tmp.width * tmp.height)) {
                continue;
            }

            if (tmp.n_channels == 3) {
                ((Rgb_t*)tmp.data)[(y1 * tmp.width) + x1]
                    = ((Rgb_t*)pixmap->data)[(y2 * pixmap->width) + x2];
            } else if (tmp.n_channels == 4) {
                ((Rgba_t*)tmp.data)[(y1 * tmp.width) + x1]
                    = ((Rgba_t*)pixmap->data)[(y2 * pixmap->width) + x2];
            }
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

/**
 * @brief Rotate the image contained in __pixmap__ 90 degrees counter-clockwise.
 * @since 07-09-2024
 * @param[in,out] pixmap The pixmap to be rotated 90 degrees counter-clockwise
 * @returns An ImcError_t representing the exit status code of the function
 */
ImcError_t imc_pixmap_rotate_ccw(Pixmap_t *pixmap) {
    Rgb_t rgb;
    Rgba_t rgba;
    Pixmap_t tmp;
    size_t x1, x2, y1, y2;

    tmp = *pixmap;
    tmp.width = pixmap->height;
    tmp.height = pixmap->width;
    tmp.data = malloc((tmp.width * imc_sizeof_px(tmp)) * tmp.height);
    if (tmp.data == NULL) {
        IMC_LOG("Failed to allocate memory for temporary pixmap buffer", IMC_ERROR);
        return IMC_EFAULT;
    }

    for (y2 = 0; y2 < pixmap->height; ++y2) {
        for (x2 = 0; x2 < pixmap->width; ++x2) {
            x1 = y2;
            y1 = -x2 + pixmap->width - 1;

            if ((y1 * tmp.width) + x1 >= (tmp.width * tmp.height)) {
                continue;
            }

            if (tmp.n_channels == 3) {
                ((Rgb_t*)tmp.data)[(y1 * tmp.width) + x1]
                    = ((Rgb_t*)pixmap->data)[(y2 * pixmap->width) + x2];
            } else if (tmp.n_channels == 4) {
                ((Rgba_t*)tmp.data)[(y1 * tmp.width) + x1]
                    = ((Rgba_t*)pixmap->data)[(y2 * pixmap->width) + x2];
            }
        }
    }

    free(pixmap->data);
    *pixmap = tmp;

    return IMC_EOK;
}

/**
 * @brief Frees resources allocated to __pixmap__.
 * @since 21-11-2024
 * @param pixmap The pixmap whos resources shall be released
 * @returns IMC_EFAULT if __pixmap__ is invalid, otherwise IMC_EOK
 */
ImcError_t imc_pixmap_destroy(Pixmap_t *pixmap) {
    if (pixmap == NULL) {
        return IMC_EFAULT;
    }

    if (pixmap->data) {
        free(pixmap->data);
    }

    free(pixmap);
    pixmap = NULL;

    return IMC_EOK;
}
