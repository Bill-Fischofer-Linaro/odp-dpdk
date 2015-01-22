/* Copyright (c) 2013, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */
/**
 * @file
 *
 * ODP debug
 */

#ifndef ODP_DEBUG_H_
#define ODP_DEBUG_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__

/**
 * Indicate deprecated variables, functions or types
 */
#define ODP_DEPRECATED __attribute__((__deprecated__))

/**
 * Intentionally unused variables ot functions
 */
#define ODP_UNUSED     __attribute__((__unused__))

#if __GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ < 6))

/**
 * _Static_assert was only added in GCC 4.6. Provide a weak replacement
 * for previous versions.
 */
#define _Static_assert(e, s) extern int (*static_assert_checker (void)) \
       [sizeof (struct { unsigned int error_if_negative: (e) ? 1 : -1; })]

#endif

#else

#define ODP_DEPRECATED
#define ODP_UNUSED

#endif

/**
 * Runtime assertion-macro - aborts if 'cond' is false.
 */
#ifndef ODP_NO_DEBUG
#define ODP_ASSERT(cond, msg) \
	do { if (!(cond)) {ODP_ERR("%s\n", msg); abort(); } } while (0)
#else
#define ODP_ASSERT(cond, msg)
#endif

/**
 * Compile time assertion-macro - fail compilation if cond is false.
 * @note This macro has zero runtime overhead
 */
#define ODP_STATIC_ASSERT(cond, msg)  _Static_assert(1, msg)

#ifdef __cplusplus
}
#endif

#endif
