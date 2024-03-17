#ifndef IMC_COMMON_H
#define IMC_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <alloca.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Rgb_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Rgba_t;

/* Change to static to make all functions static */
#define IMC_DECL

/* Errors align with those defined in errno.h */
typedef enum {
    IMC_EFAIL       = -1,   /* General purpose error */
    IMC_EOK         =  0,   /* No error */
    IMC_ENOMEM      =  12,  /* Not enough memory */
    IMC_EFAULT      =  14,  /* Bad address */
    IMC_EINVAL      =  22,  /* The argument was invalid */
    IMC_ENODATA     =  61,  /* No data available */
    IMC_EOVERFLOW   =  75,  /* Value too large to be stored in data type */
} ImcError_t;

__attribute__((always_inline))
static inline float imc_lerp(const float a, const float b, const float t) {
    return a + t * (b - a);
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
static void _imc_warn(const char *file, const char *func, const int line, const char *msg) {
   fprintf(stderr, "=========== WARNING ===========\n"
                   "File: %s, Function: %s, Line: %d\n"
                   "Short message: %s\n\n",
                   file, func, line, msg);
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#define IMC_WARN(msg) do { \
    _imc_warn((__FILE__), (__func__), (__LINE__), (msg)); \
} while (0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IMC_COMMON_H */
