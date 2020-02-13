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
 * @brief fast, secure random functions.
 *        and strong random function.
 *
 ******************************************************************************/

#ifndef _FUTILS_RANDOM_H_
#define _FUTILS_RANDOM_H_

#include <errno.h>
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
int futils_random_strong(void *buffer, size_t len);

/**
 * @brief Fill a buffer with random bytes
 *
 * @param buffer buffer to fill
 * @param len    number of bytes to fill
 */
void futils_random(void *buffer, size_t len);

/**
 * @brief Get a random uint8_t
 *
 * @return a 8 bits pseudo random value
 */
uint8_t futils_randomr8(void);

/**
 * @brief Get a random uint16_t
 *
 * @return a 16 bits pseudo random value
 */
uint16_t futils_randomr16(void);

/**
 * @brief Get a random uint32_t
 *
 * @return a 32 bits pseudo random value
 */
uint32_t futils_randomr32(void);

/**
 * @brief Get a random uint64_t
 *
 * @return a 64 bits pseudo random value
 */
uint64_t futils_randomr64(void);

/**
 * @brief Get a random uint8_t up to a maximum
 *
 * @param maximum maximum value of the returned value
 *
 * @return a 8 bits value in [0..maximum]
 */
uint8_t futils_randomr8_maximum(uint8_t maximum);

/**
 * @brief Get a random uint16_t up to a maximum
 *
 * @param maximum maximum value of the returned value
 *
 * @return a 16 bits value in [0..maximum]
 */
uint16_t futils_randomr16_maximum(uint16_t maximum);

/**
 * @brief Get a random uint32_t up to a maximum
 *
 * @param maximum maximum value of the returned value
 *
 * @return a 32 bits value in [0..maximum]
 */
uint32_t futils_randomr32_maximum(uint32_t maximum);

/**
 * @brief Get a random uint64_t up to a maximum
 *
 * @param maximum maximum value of the returned value
 *
 * @return a 64 bits value in [0..maximum]
 */
uint64_t futils_randomr64_maximum(uint64_t maximum);

/**
 * @brief Fill a buffer with random bytes
 *
 * @param buffer buffer to fill
 * @param len    number of bytes to fill
 *
 * @return 0 buffer filled with len random bytes
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random_bytes(void *buffer, size_t len)
{
	if (!buffer || !len)
		return -EINVAL;

	futils_random(buffer, len);

	return 0;
}

/**
 * @brief Get a random uint8_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random8(uint8_t *val)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr8();

	return 0;
}

/**
 * @brief Get a random uint16_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random16(uint16_t *val)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr16();

	return 0;
}

/**
 * @brief Get a random uint32_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random32(uint32_t *val)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr32();

	return 0;
}

/**
 * @brief Get a random uint64_t
 *
 * @param val pointer to randomize
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random64(uint64_t *val)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr64();

	return 0;
}

/**
 * @brief Get a random uint8_t up to a maximum
 *
 * @param val pointer to the returned 8 bits value in [0..maximum]
 * @param maximum maximum value of the returned value
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random8_maximum(uint8_t *val, uint8_t maximum)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr8_maximum(maximum);

	return 0;
}

/**
 * @brief Get a random uint16_t up to a maximum
 *
 * @param val pointer to the returned 16 bits value in [0..maximum]
 * @param maximum maximum value of the returned value
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random16_maximum(uint16_t *val, uint16_t maximum)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr16_maximum(maximum);

	return 0;
}

/**
 * @brief Get a random uint32_t up to a maximum
 *
 * @param val pointer to the returned 32 bits value in [0..maximum]
 * @param maximum maximum value of the returned value
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random32_maximum(uint32_t *val, uint32_t maximum)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr32_maximum(maximum);

	return 0;
}

/**
 * @brief Get a random uint64_t up to a maximum
 *
 * @param val pointer to the returned 64 bits value in [0..maximum]
 * @param maximum maximum value of the returned value
 *
 * @return 0 val contains a random value
 * @return -EINVAL Invalid parameter
 */
static inline int futils_random64_maximum(uint64_t *val, uint64_t maximum)
{
	if (!val)
		return -EINVAL;

	*val = futils_randomr64_maximum(maximum);

	return 0;
}

/**
 * @brief Fill a buffer with random hexadecimal characters
 *
 * @param buffer buffer to fill
 * @param len    buffer length
 * @param count  number of bytes to generate,
 *               each byte is translated to 2 hexadecimal
 *               characters stored in buffer.
 *
 * @return total number of random hexadecimal characters that
 *         would have been written to buffer if len was large
 *         enough to hold (count * 2) + 1 characters;
 *         see snprintf().
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 *
 * @note truncation will happen if len is less than
 *       (count * 2) + 1;
 *       generated string is NUL terminated if len > 0
 */
int futils_random_base16(void *buffer, size_t len, size_t count);

/**
 * @brief Fill a buffer with random base64 characters
 *
 * @param buffer      buffer to fill
 * @param len         buffer length
 * @param count       number of bytes to generate,
 *                    bytes number is rounded up to 3, then
 *                    each 3 bytes slice will be translated
 *                    to 4 base64 characters stored in buffer.
 *
 * @return total number of random base64 characters that would
 *         have been written to buffer if len was large enough
 *         to hold ((count + 2) / 3 * 4) + 1 characters;
 *         see snprintf().
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 *
 * @note truncation will happen if len is less than
 *       ((count + 2) / 3 * 4) + 1;
 *       generated string is NUL terminated if len > 0
 */
int futils_random_base64(void *buffer, size_t len, size_t count);

/**
 * @brief Shuffle the items in an array
 *
 * @param base   array to shuffle
 * @param nmemb  number of items in array
 * @param size   size of each items in the array
 *
 * @return 0 in case of success
 * @return -EINVAL Invalid parameter
 * @return other negative errno on internal error
 */
int futils_random_shuffle(void *base, size_t nmemb, size_t size);

/**
 * @brief Trigger reseeding pseudo random number generator
 *
 * Application relying on fork() should call this function
 * in child process so that new process's PRNG will produce
 * a different stream from its parent.
 *
 * This affects all functions using the PRNG, eg. all
 * except futils_random_strong().
 *
 * @return 0 state is reseed
 */
int futils_random_reseed(void);

#ifdef __cplusplus
}
#endif

#endif /* _FUTILS_RANDOM_H_ */
