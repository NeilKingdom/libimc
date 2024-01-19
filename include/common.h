#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} rgba_t;

/* Change to static to make all functions static */
#define IMC_DECL

typedef enum {
    IMC_EOK = 0, /* No error */
    IMC_ERROR,   /* General error */
    IMC_EINVAL,  /* The argument was invalid */
    IMC_ENOMEM,  /* Not enough memory */
    IMC_EFREE,   /* Error freeing memory */
    IMC_EFAULT,  /* Bad address */
    IMC_EOF      /* End of file */
} IMC_Error;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
static void _imc_warn(char *file, const char *func, int line, const char *msg) {
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

#define IMC_WARN(msg) (_imc_warn((__FILE__), (__func__), (__LINE__), (msg)))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMMON_H */
