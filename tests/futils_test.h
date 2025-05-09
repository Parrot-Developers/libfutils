/*
 * Copyright (c) 2017 Parrot S.A.
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
 * @file futils_test.h
 *
 * @brief futils test private stuff.
 *
 */

#ifndef _FUTILS_TEST_H_
#define _FUTILS_TEST_H_

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "futils/futils.h"
#include "futils/futils.hpp"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>


/** Generic log */
#define ULOG(_fmt, ...)   fprintf(stderr, _fmt "\n", ##__VA_ARGS__)
/** Log as debug */
#define ULOGD(_fmt, ...)  ULOG("[D]" _fmt, ##__VA_ARGS__)
/** Log as info */
#define ULOGI(_fmt, ...)  ULOG("[I]" _fmt, ##__VA_ARGS__)
/** Log as warning */
#define ULOGW(_fmt, ...)  ULOG("[W]" _fmt, ##__VA_ARGS__)
/** Log as error */
#define ULOGE(_fmt, ...)  ULOG("[E]" _fmt, ##__VA_ARGS__)

#endif /* _FUTILS_TEST_H_ */
