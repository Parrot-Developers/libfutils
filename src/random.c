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
 * @file random.c
 *
 * @brief strong random functions.
 *
 ******************************************************************************/

#ifdef _WIN32
#  define _CRT_RAND_S
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <ntsecapi.h>
#  ifdef _WIN64
#  define HAVE_RTLGENRANDOM 1
#  endif
#endif  /* _WIN32 */

#include "futils/random.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ULOG_TAG futils_random
#include <ulog.h>
ULOG_DECLARE_TAG(futils_random);

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ &&			\
	defined(__STDC_VERSION__) && (__STDC_VERSION >= 201112L) &&     \
	(!defined(__STDC_NO_THREADS__) || !__STDC_NO_THREADS__)
#	include <threads.h>
#	define HAVE_THREAD_LOCAL 1
#elif defined(__GNUC__) && defined(__GLIBC__)
#	define thread_local __thread
#	define HAVE_THREAD_LOCAL 1
#elif defined(_WIN32)
#	define thread_local __declspec(thread)
#	define HAVE_THREAD_LOCAL 1
#else
#	include <pthread.h>
#endif

#if defined(__GLIBC__) && \
	((__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))
#include <sys/random.h>
#define HAVE_GETRANDOM 1
#endif

static int rand_fetch(void *buffer, size_t len);

struct pool {
	unsigned int available;
	uint8_t buffer[512];
};

#ifdef HAVE_THREAD_LOCAL

static inline struct pool *pool_get(void)
{
	static thread_local struct pool pool;
	struct pool *p = &pool;

/* Workaround for GCC bug #82803:
   defeat GCC knowledge of Thread Local Storage (TLS) variable */
#if defined(__GNUC__) && !defined(__clang__)
	__asm__ ("" : "=r" (p) : "0" (p));
#endif

	return p;
}

#else /* !HAVE_THREAD_LOCAL */

static void pool_free(void *ptr)
{
	struct pool *pool = ptr;

	memset(pool, 0, sizeof(*pool));

	free(pool);
}

static struct pool *pool_new(void)
{
	struct pool *pool;

	pool = malloc(sizeof(*pool));
	if (!pool)
		return NULL;

	pool->available = 0;

	return pool;
}

static pthread_key_t pool_key;

static void pool_once(void)
{
	int err;

	err = pthread_key_create(&pool_key, pool_free);
	if (err)
		ULOG_ERRNO("pthread_key_create()", err);
}

static struct pool *pool_get(void)
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	struct pool *pool;
	int err;

	pthread_once(&once, pool_once);

	pool = pthread_getspecific(pool_key);
	if (!pool) {
		pool = pool_new();
		if (!pool)
			return NULL;

		err = pthread_setspecific(pool_key, pool);
		if (err) {
			ULOG_ERRNO("pthread_setspecific()", err);
			pool_free(pool);
			return NULL;
		}
	}

	return pool;
}

#endif /* !HAVE_THREAD_LOCAL */

/* get address of available bytes in the pool */
static inline const void *pool_buffer_get(struct pool *pool, size_t len)
{
	assert(len <= pool->available);

	return &pool->buffer[sizeof(pool->buffer) - pool->available];
}

/* consume available bytes in the pool */
static inline void pool_buffer_consume(struct pool *pool,
				       const void *ptr, size_t len)
{
	assert(ptr == &pool->buffer[sizeof(pool->buffer) - pool->available]);
	assert(len <= pool->available);

	memset(&pool->buffer[sizeof(pool->buffer) - pool->available],
	       0,
	       len);

	pool->available -= len;
}

static int pool_reload(struct pool *pool)
{
	size_t consumed;
	int err;

	consumed = sizeof(pool->buffer) - pool->available;

	/* bring remaining bytes to front */
	memmove(pool->buffer,
		&pool->buffer[consumed],
		pool->available);

	err = rand_fetch(&pool->buffer[pool->available],
			 consumed);
	if (err < 0)
		return err;

	pool->available = sizeof(pool->buffer);

	return 0;
}

static inline int pool_reload_if_needed(struct pool *pool, size_t required)
{
	int err;

	if (pool->available >= required)
		return 0;

	err = pool_reload(pool);
	if (err < 0)
		return err;

	assert(pool->available >= required);

	return 0;
}

static int pool_rand(struct pool *pool, void *buffer, size_t len)
{
	const uint8_t *ptr;
	int err;

	if (!pool)
		return -ENOMEM;

	/* if request is too large, write
	   directly to the output buffer */
	if (len >= sizeof(pool->buffer))
		return rand_fetch(buffer, len);

	/* append more bytes in the pool if
	   there's not enough byte remaining */
	err = pool_reload_if_needed(pool, len);
	if (err < 0)
		return err;

	/* extract random bytes from the random pool */
	ptr = pool_buffer_get(pool, len);

	memcpy(buffer, ptr, len);

	pool_buffer_consume(pool, ptr, len);

	return 0;
}

static int rand_fetch(void *buffer, size_t len)
{
#ifdef _WIN32
	uint8_t *p = buffer;
	int ret = 0;
	unsigned int val = 0;

#ifdef HAVE_RTLGENRANDOM
	if (!RtlGenRandom(buffer, len))
		ULOGE("RtlGenRandom");
	else
		return 0;
#endif

	while (len) {
		size_t chunk = sizeof(val);
		if (chunk > len)
			chunk = len;

		if (rand_s(&val) < 0) {
			ret = -errno;
			ULOG_ERRNO("rand_s", -ret);
			return ret;
		}

		memcpy(p, &val, chunk);

		p += chunk;
		len -= chunk;
	}
	return 0;
#else
	uint8_t *p = buffer;
	int fd;
	ssize_t rd;
	int ret = 0;

#ifdef HAVE_GETRANDOM
	while (len) {
		rd = getrandom(p, len, GRND_NONBLOCK);
		if (rd < 0) {
			if (errno == EINTR)
				continue;
			if (errno == ENOSYS)
				break;

			ULOG_ERRNO("getrandom()", errno);
			break;
		}

		if (rd == 0) {
			ULOGW("no bytes returned by getrandom(), ignoring");
			break;
		}

		p += (size_t)rd;
		len -= (size_t)rd;
	}

	if (len == 0)
		return 0;
#endif

	fd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		ret = -errno;
		ULOG_ERRNO("open(/dev/urandom)", -ret);
		return ret;
	}

	while (len) {
		rd = read(fd, p, len);
		if (rd < 0) {
			if (errno == EINTR)
				continue;
			ret = -errno;
			ULOG_ERRNO("read", -ret);
			break;
		}

		if (rd == 0) {
			ret = -EAGAIN;
			ULOGE("random_bytes: "
			      "not enough data to fill the buffer (missing %zu bytes)",
			      len);
			break;
		}

		p += (size_t)rd;
		len -= (size_t)rd;
	}

	close(fd);
	return ret;
#endif
}

int futils_random_bytes(void *buffer, size_t len)
{
	struct pool *pool = pool_get();

	if (!buffer || len == 0)
		return -EINVAL;

	return pool_rand(pool, buffer, len);
}

int futils_random8(uint8_t *val)
{
	return futils_random_bytes(val, sizeof(*val));
}

int futils_random16(uint16_t *val)
{
	return futils_random_bytes(val, sizeof(*val));
}

int futils_random32(uint32_t *val)
{
	return futils_random_bytes(val, sizeof(*val));
}

int futils_random64(uint64_t *val)
{
	return futils_random_bytes(val, sizeof(*val));
}
