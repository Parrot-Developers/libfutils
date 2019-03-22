/******************************************************************************
 * Copyright (c) 2019 Parrot S.A.
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
 * @file random.h
 *
 * @brief strong random functions.
 *
 ******************************************************************************/

#ifndef _FUTILS_RANDOM_H_
#define _FUTILS_RANDOM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fill a buffer with random bytes
 *
 * @param buffer buffer to fill
 * @param len    number of bytes to fill
 *
 * @return 0 buffer filled with len random bytes
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 */
int futils_random_bytes(void *buffer, size_t len);

/**
 * @brief Get a random uint8_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 */
int futils_random8(uint8_t *val);

/**
 * @brief Get a random uint16_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 */
int futils_random16(uint16_t *val);

/**
 * @brief Get a random uint32_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 */
int futils_random32(uint32_t *val);

/**
 * @brief Get a random uint64_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 */
int futils_random64(uint64_t *val);

#ifdef __cplusplus
}
#endif

#endif /* _FUTILS_RANDOM_H_ */