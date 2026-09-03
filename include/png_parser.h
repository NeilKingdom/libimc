#ifndef PNG_PARSER_H
#define PNG_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "imc_common.h"
#include "pixmap.h"

#define NONE 0
#define PNG_MAGIC ((u8[8]){ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A })

/**
 * =========================================================
 * Critical Chunks - Must appear in order. PLTE is optional.
 * =========================================================
 * Name     Multiple OK?    Ordering constraints
 *
 * IHDR     No              Must be first
 * PLTE     No              Before IDAT
 * IDAT     Yes             Multiple IDATs must be consecutive
 * IEND     No              Must be last
 *
 * =========================================================
 * Ancillary Chunks - Need not appear in order.
 * =========================================================
 * Name     Multiple OK?    Ordering constraints
 *
 * cHRM     No              Before PLTE and IDAT
 * gAMA     No              Before PLTE and IDAT
 * iCCP     No              Before PLTE and IDAT
 * sBIT     No              Before PLTE and IDAT
 * sRGB     No              Before PLTE and IDAT
 * bKGD     No              After PLTE; before IDAT
 * hIST     No              After PLTE; before IDAT
 * pHYs     No              After PLTE; before IDAT
 * sPLT     Yes             Before IDAT
 * tIME     No              None
 * iTXt     Yes             None
 * tEXt     Yes             None
 * zTXt     Yes             None
 */

/*** Critical chunks ***/

#define IHDR "IHDR" // Image header
#define PLTE "PLTE" // Palette
#define IDAT "IDAT" // Image data
#define IEND "IEND" // Image trailer

/*** Ancillary chunks ***/

#define TRNS "tRNS" // Transparency
#define GAMA "gAMA" // Image gamma
#define CHRM "cHRM" // Primary chromaticities
#define SRGB "sRGB" // Standard RGB color space
#define ICCP "iCCP" // Embedded ICC profile

// Textual information
#define TEXT "tEXt" // Textual data
#define ZTXT "zTXt" // Compressed textual data
#define ITXT "iTXt" // International textual data

// Misc information
#define BKGD "bKGD" // Background color
#define PHYS "pHYs" // Physical pixel dimensions
#define SBIT "sBIT" // Significant bits
#define SPLT "sPLT" // Suggested palette
#define HIST "hIST" // Palette histogram
#define TIME "tIME" // Image last-modification time

typedef struct {
    u32  length;  // Length of data segment
    u32  crc;     // Cyclic Redundancy Check
    u8  *data;    // Chunk data (variable size)
    char type[4]; // Chunk type code
} Chunk_t;

/**
 * =============================================================
 * Image type        Color type      Bit depths      Explanation
 * =============================================================
 *
 * Greyscale           0             1, 2, 4, 8, 16  Each pixel is a greyscale sample
 * Truecolor           2             8, 6            Each pixel is an RGB triple
 * Indexed-color       3             1, 2, 4, 8      Each pixel is a palette index (see PLTE)
 * Greyscale + alpha   4             8, 16           Each pixel is a greyscale sample followed by alpha sample
 * Truecolor + alpha   6             8, 16           Each pixel is an RGB triple followed by alpha sample
 */
typedef enum {
    PALETTE = 0x01 << 0,
    COLOR   = 0x01 << 1,
    ALPHA   = 0x01 << 2,
} ColorType_t;

typedef struct {
    u32 width;             // Image width (in pixels)
    u32 height;            // Image height (in pixels)
    u8  bit_depth;         // Number of bits per-channel
    u8  color_type;        // See enum ColorType_t
    u8  compress_mthd;     // Compression method (currently only 0 is supported (LZ77))
    u8  filter_mthd;       // Filter method (currently only 0 is supported (Adaptive filtering))
    u8  interlace_mthd;    // Interlace method (0 = Non-interlaced, 1 = interlaced)
    u8  n_channels;        // The number of channels / samples per pixel
} Ihdr_t;

/* NOT SUPPORTED
typedef struct {
    Rgb_t entries[256];
} Plte_t;
*/

typedef struct {
    u8   *data;     // Compressed data stream
    usize length;   // Length of compressed data stream
    usize offset;   // Offset into the buffer (used when appending multiple IDAT chunks)
} Idat_t;

typedef struct {
} Trns_t;

typedef struct {
} Gama_t;

typedef struct {
    u32 wpoint_x;  // White-point x
    u32 wpoint_y;  // White-point y
    u32 red_x;     // Red x
    u32 red_y;     // Red y
    u32 green_x;   // Green x
    u32 green_y;   // Green y
    u32 blue_x;    // Blue x
    u32 blue_y;    // Blue y
} Chrm_t;

typedef enum {
    PERCEPTUAL,     // Perceptual
    RELATIVE,       // Relative colorimetric
    SATURATION,     // Saturation
    ABSOLUTE        // Absolute colorimetric
} RenderIntent_t;

typedef struct {
    RenderIntent_t ri;
} Srgb_t;

typedef struct {
    char profile_name[79];
    u8   compress_mthd;
    u8  *comp_profile;
} Iccp_t;

typedef enum {
    SUB = 1,
    UP,
    AVG,
    PAETH
} FilterMethod_t;

typedef struct {
    FILE *fp;    // The file handle
    u8   *data;  // Copy of raw data
    usize size;  // Size of data (in bytes)
} PngHndl_t;

// Used for reconstructing filtered scanlines
typedef u8 (*reconstruct_fn)(
    u8 *prev_scanline,
    u8 *curr_scanline,
    const u8 n_channels,
    const usize idx
);

// Forward function declarations

PngHndl_t      *imc_png_open(const char* const path);
ImcResult_t     imc_png_close(PngHndl_t *png);
Pixmap_t       *imc_png_parse(PngHndl_t *png);

#ifdef __cplusplus
}
#endif

#endif
