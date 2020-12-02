/******************************************************************************
 * Copyright (c) 2015 Parrot S.A.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Parrot Company nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE PARROT COMPANY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file futils.h
 *
 * @brief utility C functions & macro
 *
 *****************************************************************************/

#ifndef _FUTILS_H_
#define _FUTILS_H_

/**
 * macro used to retrieve number of element of a static fixed array
 */
#define FUTILS_SIZEOF_ARRAY(x) (sizeof((x)) / sizeof((x)[0]))

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(x) FUTILS_SIZEOF_ARRAY(x)
#endif

/**
 * macro used to stringify text
 */
#define FUTILS_STRINGIFY(x) #x
#define FUTILS_TOSTRING(x) FUTILS_STRINGIFY(x)

#ifndef TOSTRING
#define TOSTRING(x) FUTILS_TOSTRING(x)
#endif

/**
 * MIN and MAX macro
 */
#define FUTILS_MIN(a, b) \
	({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })

#ifndef MIN
#define MIN(a, b) FUTILS_MIN(a, b)
#endif

#define FUTILS_MAX(a, b) \
	({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })

#ifndef MAX
#define MAX(a, b) FUTILS_MAX(a, b)
#endif

#define FUTILS_BOUND(a, m, M) \
	({ __typeof__(a) _a = (a); __typeof__(m) _m = (m); \
		__typeof__(M) _M = (M); _a < _m ? _m : _a > _M ? _M : _a; })

#ifndef BOUND
#define BOUND(a, m, M) FUTILS_BOUND(a, m, M)
#endif

/**
 * STATIC_ASSERT() can be used to perform many compile-time assertions:
 * type sizes, etc...
 */
#ifdef __COVERITY__
#  define FUTILS_STATIC_ASSERT(x, msg)
#elif __STDC_VERSION__ >= 201112L
# include <assert.h>
#  define FUTILS_STATIC_ASSERT(x, msg) \
	static_assert(x, msg)
#else
#  define FUTILS_STATIC_ASSERT(x, msg) \
	/* codecheck_ignore[NEW_TYPEDEFS] */ \
	typedef char __STATIC_ASSERT__[(x) ? 1 : -1]
#endif

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(x, msg) FUTILS_STATIC_ASSERT(x, msg)
#endif

/** Wrapper for gcc printf attribute */
#ifndef FUTILS_ATTRIBUTE_FORMAT_PRINTF
#  if defined(__GNUC__) && defined(__MINGW32__) && !defined(__clang__)
#    define FUTILS_ATTRIBUTE_FORMAT_PRINTF(_x, _y) \
		__attribute__((__format__(__gnu_printf__, _x, _y)))
#  elif defined(__GNUC__)
#    define FUTILS_ATTRIBUTE_FORMAT_PRINTF(_x, _y) \
		__attribute__((__format__(__printf__, _x, _y)))
#  else
#    define FUTILS_ATTRIBUTE_FORMAT_PRINTF(_x, _y)
#  endif
#endif /* !FUTILS_ATTRIBUTE_FORMAT_PRINTF */

/**
 * include libfutils headers
 **/
#include <futils/fdutils.h>
#include <futils/hash.h>
#include <futils/list.h>
#include <futils/timetools.h>
#include <futils/systimetools.h>
#include <futils/synctools.h>
#include <futils/mbox.h>
#include <futils/dynmbox.h>
#include <futils/random.h>
#include <futils/varint.h>
#include <futils/safew.h>
#include <futils/futils.hpp>

#endif /*_FUTILS_H_ */
