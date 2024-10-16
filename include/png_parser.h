#ifndef PNG_PARSER_H
#define PNG_PARSER_H

/* Required for fileno() */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

#include <zlib.h>
#include <sys/stat.h>

#include "imc_common.h"
#include "pixmap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NONE 0
#define PNG_MAGIC ((uint8_t[8]){ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A })

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

/* Critical chunks */
#define IHDR "IHDR" /* Image header */
#define PLTE "PLTE" /* Palette */
#define IDAT "IDAT" /* Image data */
#define IEND "IEND" /* Image trailer */

/* Ancillary chunks */
#define TRNS "tRNS" /* Transparency */
#define GAMA "gAMA" /* Image gamma */
#define CHRM "cHRM" /* Primary chromaticities */
#define SRGB "sRGB" /* Standard RGB color space */
#define ICCP "iCCP" /* Embedded ICC profile */

/* Textual information */
#define TEXT "tEXt" /* Textual data */
#define ZTXT "zTXt" /* Compressed textual data */
#define ITXT "iTXt" /* International textual data */

/* Misc information */
#define BKGD "bKGD" /* Background color */
#define PHYS "pHYs" /* Physical pixel dimensions */
#define SBIT "sBIT" /* Significant bits */
#define SPLT "sPLT" /* Suggested palette */
#define HIST "hIST" /* Palette histogram */
#define TIME "tIME" /* Image last-modification time */

typedef struct {
    uint32_t length;    /* Length of data segment */
    uint32_t crc;       /* Cyclic Redundancy Check */
    uint8_t *data;      /* Chunk data (variable size) */
    char     type[4];   /* Chunk type code */
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
    PALETTE = 0x01,
    COLOR   = 0x02,
    ALPHA   = 0x04
} ColorType_t;

typedef struct {
    uint32_t width;             /* Image width (in pixels) */
    uint32_t height;            /* Image height (in pixels) */
    uint8_t  bit_depth;         /* Number of bits per-channel */
    uint8_t  color_type;        /* See enum ColorType_t */
    uint8_t  compress_mthd;     /* Compression method (currently only 0 is supported (LZ77)) */
    uint8_t  filter_mthd;       /* Filter method (currently only 0 is supported (Adaptive filtering)) */
    uint8_t  interlace_mthd;    /* Interlace method (0 = Non-interlaced, 1 = interlaced) */
    uint8_t  n_channels;        /* The number of channels / samples per pixel */
} Ihdr_t;

/* NOT SUPPORTED
typedef struct {
    Rgb_t entries[256];
} Plte_t;
*/

typedef struct {
    uint8_t *data;     /* Compressed data stream */
    size_t   length;   /* Length of compressed data stream */
    size_t   offset;   /* Offset into the buffer (used when appending multiple IDAT chunks) */
} Idat_t;

typedef struct {
} Trns_t;

typedef struct {
} Gama_t;

typedef struct {
    uint32_t wpoint_x;  /* White-point x */
    uint32_t wpoint_y;  /* White-point y */
    uint32_t red_x;     /* Red x */
    uint32_t red_y;     /* Red y */
    uint32_t green_x;   /* Green x */
    uint32_t green_y;   /* Green y */
    uint32_t blue_x;    /* Blue x */
    uint32_t blue_y;    /* Blue y */
} Chrm_t;

typedef enum {
    PERCEPTUAL,     /* Perceptual */
    RELATIVE,       /* Relative colorimetric */
    SATURATION,     /* Saturation */
    ABSOLUTE        /* Absolute colorimetric */
} RenderIntent_t;

typedef struct {
    RenderIntent_t ri;
} Srgb_t;

typedef struct {
    char     profile_name[79];
    uint8_t  compress_mthd;
    uint8_t *comp_profile;
} Iccp_t;

typedef enum {
    SUB = 1,
    UP,
    AVG,
    PAETH
} FilterMethod_t;

typedef struct {
    FILE    *fp;    /* The file handle */
    uint8_t *data;  /* Copy of raw data */
    size_t   size;  /* Size of data (in bytes) */
} PngHndl_t;

/* Used for reconstructing filtered scanlines */
typedef uint8_t (*recon_func)(
    uint8_t *prev_scanline,
    uint8_t *curr_scanline,
    const uint8_t n_channels,
    const size_t idx
);

/* Forward function declarations */

PngHndl_t      *imc_png_open(const char* const path);
ImcError_t      imc_png_close(PngHndl_t *png);
Pixmap_t       *imc_png_parse(PngHndl_t *png);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PNG_PARSER_H */
