#ifndef PNG_PARSER_H
#define PNG_PARSER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PNG_MAGIC ((uint8_t[8]){ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A })

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

typedef struct Chunk {
    uint32_t length;    /* Length of data segment */
    uint32_t crc;       /* Cyclic Redundancy Check */
    uint8_t *data;      /* Chunk data (variable size) */
    char     type[4];   /* Chunk type code */
} chunk_t;

typedef enum {
    NONE    = (uint8_t)0,
    PALETTE = (uint8_t)1,
    COLOR   = (uint8_t)2, 
    ALPHA   = (uint8_t)4
} ColorType;

typedef struct {
    uint32_t width;             /* Image widht */
    uint32_t height;            /* Image height */
    uint8_t  bit_depth;         /* Pixel bit-depth */
    uint8_t  color_type;        /* See enum ColorType */
    uint8_t  compress_mthd;     /* Currently only 0 (LZ77) */
    uint8_t  filter_mthd;       /* Currently onlt 0 (Adaptive filtering) */
    uint8_t  interlace_mthd;    /* 0 = Non-interlaced, 1 = interlaced */
    uint8_t  padding[3];        /* Padding for struct bit alignment */
} ihdr_t;

typedef struct ColorRGB {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_rgb_t;

typedef struct {
    color_rgb_t plte_entries[256];
} plte_t;

typedef struct {
    int placeholder;
} idat_t;

typedef struct {

} trns_t;

typedef struct {

} gama_t;

typedef struct {
    uint32_t wpoint_x;
    uint32_t wpoint_y;
    uint32_t red_x;
    uint32_t red_y;
    uint32_t green_x;
    uint32_t green_y;
    uint32_t blue_x;
    uint32_t blue_y;
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

typedef struct {
    FILE    *fp;    /* The file handle */
    uint8_t *data;  /* Copy of raw data */
    size_t   size;  /* Size of data in bytes */
} *png_hndl_t;

png_hndl_t  imc_open_png(const char *path);
IMC_Error   imc_close_png(png_hndl_t png);
IMC_Error   imc_parse_png(png_hndl_t png);
IMC_Error   imc_destroy_png(png_hndl_t png);

#ifdef __cplusplus
}
#endif

#endif /* PNG_PARSER_H */
