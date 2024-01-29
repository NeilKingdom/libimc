#ifndef PNG_PARSER_H
#define PNG_PARSER_H

/* Required for fileno() */
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif 

#include "common.h"
#include "pixmap.h"

#include <zlib.h>
#include <sys/stat.h> 

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
} chunk_t;

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
    PALETTE = 1,
    COLOR   = 2, 
    ALPHA   = 4
} ColorType;

typedef struct {
    uint32_t width;             /* Image width */
    uint32_t height;            /* Image height */
    uint8_t  bit_depth;         /* Pixel bit-depth */
    uint8_t  color_type;        /* See enum ColorType */
    uint8_t  compress_mthd;     /* Currently only 0 (LZ77) */
    uint8_t  filter_mthd;       /* Currently onlt 0 (Adaptive filtering) */
    uint8_t  interlace_mthd;    /* 0 = Non-interlaced, 1 = interlaced */
    uint8_t  n_channels;        /* The number of channels / samples per pixel */
    uint8_t  padding[2];        /* Padding for struct bit alignment */
} ihdr_t;

typedef struct {
    rgb_t plte_entries[256];
} plte_t;

typedef struct {
    uint8_t *data;     /* Compressed data stream */
    size_t   length;   /* Length of compressed data stream */
    size_t   offset;   /* Offset into the buffer (used when appending multiple IDAT chunks) */
} idat_t;

typedef struct {

} trns_t;

typedef struct {

} gama_t;

typedef struct {
    uint32_t wpoint_x;  /* White-point x */
    uint32_t wpoint_y;  /* White-point y */
    uint32_t red_x;     /* Red x */
    uint32_t red_y;     /* Red y */
    uint32_t green_x;   /* Green x */
    uint32_t green_y;   /* Green y */
    uint32_t blue_x;    /* Blue x */
    uint32_t blue_y;    /* Blue y */
} chrm_t;

typedef enum {
    PERCEPTUAL,     /* Perceptual */
    RELATIVE,       /* Relative colorimetric */
    SATURATION,     /* Saturation */ 
    ABSOLUTE        /* Absolute colorimetric */
} RenderIntent;

typedef struct {
    RenderIntent ri;    /* See enum RenderIntent */
} srgb_t;

typedef struct {
    char     profile_name[79];
    uint8_t  compress_mthd;
    uint8_t *comp_profile;
} iccp_t;

typedef enum {
    SUB = 1,   
    UP,     
    AVG,    
    PAETH
} FilterMethod;

typedef struct {
    FILE    *fp;    /* The file handle */
    uint8_t *data;  /* Copy of raw data */
    size_t   size;  /* Size of data in bytes */
} *png_hndl_t;

/* Used for reconstructing filtered scanlines */
typedef uint8_t (*recon_func)(
    uint8_t *prev_scanline, 
    uint8_t *curr_scanline, 
    const uint8_t n_channels, 
    const size_t idx
);

/* Forward function decls */

png_hndl_t  imc_open_png(const char *path);
IMC_Error   imc_close_png(png_hndl_t png);
IMC_Error   imc_parse_png(png_hndl_t png);
IMC_Error   imc_destroy_png(png_hndl_t png);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PNG_PARSER_H */
