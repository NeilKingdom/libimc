#include "imc_common.h"

/**
 * @brief Blends __fg_col__ with __bg_col__ accounting for the opacity given by __alpha__.
 * @since 15-01-2024
 * @param[in] fg_col The foreground color
 * @param[in] bg_col The background color
 * @param[in] alpha The opacity/alpha value that will be factored when blending
 * @returns A new Rgb_t containing the blended rgb pixel value
 */
static Rgb_t imc_blend_alpha(const Rgb_t fg_col, const Rgb_t bg_col, const u8 alpha) {
    u8 r, g, b;
    f32 a = (f32)alpha / 255.0f;
    r = (1.0f - a) * bg_col.r + a * fg_col.r;
    g = (1.0f - a) * bg_col.g + a * fg_col.g;
    b = (1.0f - a) * bg_col.b + a * fg_col.b;

    return (Rgb_t){ r, g, b };
}
