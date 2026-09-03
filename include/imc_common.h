#ifndef IMC_COMMON_H
#define IMC_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <alloca.h>
#include <math.h>
#include <errno.h>
#include <zlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "ezc.h"

typedef struct {
    u8 r, g, b;
} RGB8_t;

typedef struct {
    u8 r, g, b, a;
} RGBA8_t;

typedef enum {
    RGB  = 3,
    RGBA = 4,
} PixFormat_t;

typedef enum {
    IMC_FAILURE  = -1,         // General purpose error
    IMC_SUCCESS  =  0,         // Success
    IMC_NOMEM    =  ENOMEM,    // Not enough memory
    IMC_FAULT    =  EFAULT,    // Bad address
    IMC_INVAL    =  EINVAL,    // The argument was invalid
    IMC_NODATA   =  ENODATA,   // No data available
    IMC_OVERFLOW =  EOVERFLOW, // Value too large to be stored in data type
} ImcResult_t;

typedef enum {
    IMC_INFO,
    IMC_WARN,
    IMC_ERROR
} ImcLogLevel_t;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

static void _imc_log(
    const char *file,
    const char *func,
    const i32 line,
    const char *msg,
    const ImcLogLevel_t level
) {
    char *header;
    switch (level) {
        case IMC_INFO:
            header = "INFO";
            break;
        case IMC_WARN:
            header = "WARNING";
            break;
        case IMC_ERROR:
            header = "ERROR";
            break;
    }

    fprintf((level == IMC_INFO) ? stdout : stderr,
        "\n=========== %s ===========\n"
        "Logger: %s\n"
        "File: %s, Function: %s, Line: %d\n",
        header, msg, file, func, line
    );
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#define IMC_LOG(msg, level) do { \
    _imc_log((__FILE__), (__func__), (__LINE__), (msg), (level)); \
} while (0)

#ifdef __cplusplus
}
#endif

#endif
