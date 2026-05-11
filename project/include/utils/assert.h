/**
 * @file utils/assert.h
 * @brief Assertion macros that log before aborting.
 *
 * GN_ASSERT(cond, msg...) is a hard runtime assertion (always evaluated,
 * even in release builds). GN_DEBUG_ASSERT is compiled out in NDEBUG builds.
 */
#pragma once

#include "utils/logger.h"

#include <cstdlib>

#define GN_ASSERT(cond, ...)                                              \
    do {                                                                  \
        if (!(cond)) {                                                    \
            GN_FATAL("Assertion failed: {} :: " __VA_ARGS__, #cond);      \
            std::abort();                                                 \
        }                                                                 \
    } while (0)

#ifndef NDEBUG
    #define GN_DEBUG_ASSERT(cond, ...) GN_ASSERT(cond, __VA_ARGS__)
#else
    #define GN_DEBUG_ASSERT(cond, ...) ((void)0)
#endif

#define GN_UNREACHABLE(msg)                       \
    do {                                          \
        GN_FATAL("Unreachable: {}", msg);         \
        std::abort();                             \
    } while (0)
