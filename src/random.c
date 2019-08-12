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
 * @brief fast, secure random functions.
 *        and strong random function.
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
#include <limits.h>
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

/*
 * return a power of 2 minus one number
 * at least equal to parameter if parameter
 * is not 0. return 0 otherwise
 */
static inline uint64_t p2minus1(uint64_t v)
{
	v |= (v >> 1);
	v |= (v >> 2);
	v |= (v >> 4);
	v |= (v >> 8);
	v |= (v >> 16);
	v |= (v >> 32);

	return v;
}

/*
 * returns the number of bits required to
 * represent 'v' aka. the position of its most
 * significant bit set (with less significant
 * bit starting at 1):
 * 0b0000 -> 0
 * 0b0001 -> 1
 * 0b0010 -> 2
 * 0b0011 -> 2
 * 0b0100 -> 3
 * ...
 * 0b0111 -> 3
 * 0b1000 -> 4
 * ...
 * 0b1111 -> 4
 */
static inline unsigned int ilog2plus1(uint64_t v)
{
	unsigned int r = 0;

	if (v >= (UINT64_C(1) << 32)) {
		r += 33;
		v >>= 33;
	}

	if (v >= (UINT32_C(1) << 16)) {
		r += 17;
		v >>= 17;
	}

	if (v >= (UINT16_C(1) << 8)) {
		r += 9;
		v >>= 9;
	}

	if (v >= (UINT8_C(1) << 4)) {
		r += 5;
		v >>= 5;
	}

	if (v >= (UINT8_C(1) << 2)) {
		r += 3;
		v >>= 3;
	}

	if (v >= (UINT8_C(1) << 1)) {
		r += 2;
		v >>= 2;
	}

	if (v >= (UINT8_C(1) << 0)) {
		r += 1;
		v >>= 1;
	}

	return r;
}

static inline uint32_t rotl(uint32_t x, unsigned int k)
{
	return (x << k) | (x >> (32 - k));
}

static inline void read_32le(const uint8_t *d, uint32_t *v)
{
	*v = 0;
	*v |= (uint32_t)d[0] <<	 0;
	*v |= (uint32_t)d[1] <<	 8;
	*v |= (uint32_t)d[2] << 16;
	*v |= (uint32_t)d[3] << 24;
}

static inline void write_32le(const uint32_t v, uint8_t *d)
{
	d[0] = (v >>  0);
	d[1] = (v >>  8);
	d[2] = (v >> 16);
	d[3] = (v >> 24);
}

#define CHACHA_KEY_SIZE 32
#define CHACHA_NONCE_SIZE 12
#define CHACHA_KEY_NONCE_SIZE (CHACHA_KEY_SIZE + CHACHA_NONCE_SIZE)
#define CHACHA_BLOCK_SIZE 64

#define CHACHA_ROUNDS 20

static inline void chacha_quarterround(uint32_t x[16],
				       const unsigned int a,
				       const unsigned int b,
				       const unsigned int c,
				       const unsigned int d)
{
	x[a] += x[b]; x[d] = rotl(x[d] ^ x[a], 16);
	x[c] += x[d]; x[b] = rotl(x[b] ^ x[c], 12);
	x[a] += x[b]; x[d] = rotl(x[d] ^ x[a], 8);
	x[c] += x[d]; x[b] = rotl(x[b] ^ x[c], 7);
}

static inline void chacha_block(const uint32_t in[16],
				uint8_t out[CHACHA_BLOCK_SIZE])
{
	unsigned int i;
	uint32_t x[16];

	for (i = 0; i < 16; i++)
		x[i] = in[i];

	for (i = 0; i < CHACHA_ROUNDS; i += 2) {
		chacha_quarterround(x, 0, 4,  8, 12);
		chacha_quarterround(x, 1, 5,  9, 13);
		chacha_quarterround(x, 2, 6, 10, 14);
		chacha_quarterround(x, 3, 7, 11, 15);
		chacha_quarterround(x, 0, 5, 10, 15);
		chacha_quarterround(x, 1, 6, 11, 12);
		chacha_quarterround(x, 2, 7,  8, 13);
		chacha_quarterround(x, 3, 4,  9, 14);
	}

	for (i = 0; i < 16; i++)
		x[i] += in[i];

	for (i = 0; i < 16; i++)
		write_32le(x[i], &out[i * 4]);
}

/* chacha20 state */
struct chacha {
	uint32_t x[16];
};

static void chacha_init(struct chacha *chacha,
			const uint8_t k[CHACHA_KEY_NONCE_SIZE])
{
	static const uint8_t c[16] = "expand 32-byte k";

	/* constant */
	read_32le(&c[0], &chacha->x[0]);
	read_32le(&c[4], &chacha->x[1]);
	read_32le(&c[8], &chacha->x[2]);
	read_32le(&c[12], &chacha->x[3]);

	/* key */
	read_32le(&k[0], &chacha->x[4]);
	read_32le(&k[4], &chacha->x[5]);
	read_32le(&k[8], &chacha->x[6]);
	read_32le(&k[12], &chacha->x[7]);
	read_32le(&k[16], &chacha->x[8]);
	read_32le(&k[20], &chacha->x[9]);
	read_32le(&k[24], &chacha->x[10]);
	read_32le(&k[28], &chacha->x[11]);

	/* counter */
	chacha->x[12] = 0;

	/* nonce */
	read_32le(&k[32], &chacha->x[13]);
	read_32le(&k[36], &chacha->x[14]);
	read_32le(&k[40], &chacha->x[15]);
}

/* get one block of chacha20 keystream */
static inline void chacha_get(struct chacha *chacha,
			      uint8_t out[CHACHA_BLOCK_SIZE])
{
	chacha_block(chacha->x, out);

	/* counter */
	chacha->x[12]++;
}

static void chacha_keystream(struct chacha *chacha,
			     void *buffer, size_t len)
{
	uint8_t *p = buffer;

	/* full blocks */
	while (len >= CHACHA_BLOCK_SIZE) {

		chacha_get(chacha, p);

		p += CHACHA_BLOCK_SIZE;
		len -= CHACHA_BLOCK_SIZE;
	}

	/* last partial block */
	if (len) {
		uint8_t tmp[CHACHA_BLOCK_SIZE];

		chacha_get(chacha, tmp);

		memcpy(p, tmp, len);

		memset(tmp, 0, sizeof(tmp));
	}
}

static int rand_fetch(void *buffer, size_t len);

struct pool {
	struct chacha chacha;
	unsigned int seeded;
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

	pool->seeded = 0;
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

static int pool_seed(struct pool *pool)
{
	uint8_t key[CHACHA_KEY_NONCE_SIZE];
	int err;

	err = rand_fetch(key, sizeof(key));
	if (err) {
		ULOG_ERRNO("rand_fetch()",
			   -err);
		return err;
	}

	chacha_init(&pool->chacha, key);

	pool->seeded = 1;

	memset(key, 0, sizeof(key));

	return 0;
}

static inline int pool_seed_if_needed(struct pool *pool)
{
	/* seed if needed */
	if (pool->seeded)
		return 0;

	return pool_seed(pool);
}

static void pool_reload(struct pool *pool)
{
	size_t consumed;
	const uint8_t *key;

	consumed = sizeof(pool->buffer) - pool->available;

	/* bring remaining bytes to front */
	memmove(pool->buffer,
		&pool->buffer[consumed],
		pool->available);

	/* fill pool buffer with pseudorandom bytes */
	chacha_keystream(&pool->chacha,
			 &pool->buffer[pool->available],
			 consumed);

	pool->available = sizeof(pool->buffer);

	/* apply a new key for backtracking protection */
	key = pool_buffer_get(pool, CHACHA_KEY_NONCE_SIZE);

	chacha_init(&pool->chacha, key);

	pool_buffer_consume(pool, key, CHACHA_KEY_NONCE_SIZE);
}

static inline void pool_reload_if_needed(struct pool *pool,
					 size_t required)
{
	if (pool->available >= required)
		return;

	pool_reload(pool);

	assert(pool->available >= required);
}

static void pool_stir(struct pool *pool, void *buffer, size_t len)
{
	struct chacha chacha;
	const uint8_t *key;

	/* get enough bytes for a new dedicated key */
	pool_reload_if_needed(pool, CHACHA_KEY_NONCE_SIZE);

	key = pool_buffer_get(pool, CHACHA_KEY_NONCE_SIZE);

	chacha_init(&chacha, key);

	pool_buffer_consume(pool, key, CHACHA_KEY_NONCE_SIZE);

	/* stir key to fill buffer */
	chacha_keystream(&chacha, buffer, len);

	memset(&chacha, 0, sizeof(chacha));
}

static int pool_rand(struct pool *pool, void *buffer, size_t len)
{
	const uint8_t *ptr;
	int err;

	if (!pool)
		return -ENOMEM;

	err = pool_seed_if_needed(pool);
	if (err < 0)
		return err;

	/* if request is too large, write
	   directly to the output buffer */
	if (len >= sizeof(pool->buffer) - CHACHA_KEY_NONCE_SIZE) {
		pool_stir(pool, buffer, len);
		return 0;
	}

	/* append more bytes in the pool if
	   there's not enough byte remaining */
	pool_reload_if_needed(pool, len);

	/* extract random bytes from the random pool */
	ptr = pool_buffer_get(pool, len);

	memcpy(buffer, ptr, len);

	pool_buffer_consume(pool, ptr, len);

	return 0;
}

static int pool_rand8(struct pool *pool, uint8_t *out)
{
	return pool_rand(pool, out, sizeof(*out));
}

static int pool_rand16(struct pool *pool, uint16_t *out)
{
	return pool_rand(pool, out, sizeof(*out));
}

static int pool_rand24(struct pool *pool, uint32_t *out)
{
	uint8_t bytes[3];
	int err;

	err = pool_rand(pool, bytes, sizeof(bytes));
	if (err < 0)
		return err;

	*out = 0;
	*out |= (uint32_t)bytes[0] << 0;
	*out |= (uint32_t)bytes[1] << 8;
	*out |= (uint32_t)bytes[2] << 16;

	return 0;
}

static int pool_rand32(struct pool *pool, uint32_t *out)
{
	return pool_rand(pool, out, sizeof(*out));
}

static int pool_rand40(struct pool *pool, uint64_t *out)
{
	uint8_t bytes[5];
	int err;

	err = pool_rand(pool, bytes, sizeof(bytes));
	if (err < 0)
		return err;

	*out = 0;
	*out |= (uint64_t)bytes[0] << 0;
	*out |= (uint64_t)bytes[1] << 8;
	*out |= (uint64_t)bytes[2] << 16;
	*out |= (uint64_t)bytes[3] << 24;
	*out |= (uint64_t)bytes[4] << 32;

	return 0;
}

static int pool_rand48(struct pool *pool, uint64_t *out)
{
	uint8_t bytes[6];
	int err;

	err = pool_rand(pool, bytes, sizeof(bytes));
	if (err < 0)
		return err;

	*out = 0;
	*out |= (uint64_t)bytes[0] << 0;
	*out |= (uint64_t)bytes[1] << 8;
	*out |= (uint64_t)bytes[2] << 16;
	*out |= (uint64_t)bytes[3] << 24;
	*out |= (uint64_t)bytes[4] << 32;
	*out |= (uint64_t)bytes[5] << 40;

	return 0;
}

static int pool_rand56(struct pool *pool, uint64_t *out)
{
	uint8_t bytes[7];
	int err;

	err = pool_rand(pool, bytes, sizeof(bytes));
	if (err < 0)
		return err;

	*out = 0;
	*out |= (uint64_t)bytes[0] << 0;
	*out |= (uint64_t)bytes[1] << 8;
	*out |= (uint64_t)bytes[2] << 16;
	*out |= (uint64_t)bytes[3] << 24;
	*out |= (uint64_t)bytes[4] << 32;
	*out |= (uint64_t)bytes[5] << 40;
	*out |= (uint64_t)bytes[6] << 48;

	return 0;
}

static int pool_rand64(struct pool *pool, uint64_t *out)
{
	return pool_rand(pool, out, sizeof(*out));
}

#define _pool_rand_maximum(pool, bits, maximum, mask, out, err) do {	\
	*(err) = 0;							\
	do {								\
		*(err) = pool_rand ## bits((pool), (out));		\
		if (*(err) < 0)						\
			break;						\
		*(out) &= (mask);					\
	} while (*(out) > (maximum));					\
} while (0)

static int _pool_rand8_maximum(struct pool *pool,
			       uint8_t maximum, uint8_t mask, uint8_t *out)
{
	int err;

	_pool_rand_maximum(pool, 8, maximum, mask, out, &err);

	return err;
}

static int _pool_rand16_maximum(struct pool *pool,
				uint16_t maximum, uint16_t mask, uint16_t *out)
{
	int err;

	_pool_rand_maximum(pool, 16, maximum, mask, out, &err);

	return err;
}

static int _pool_rand24_maximum(struct pool *pool,
				uint32_t maximum, uint32_t mask, uint32_t *out)
{
	int err;

	_pool_rand_maximum(pool, 24, maximum, mask, out, &err);

	return err;
}

static int _pool_rand32_maximum(struct pool *pool,
				uint32_t maximum, uint32_t mask, uint32_t *out)
{
	int err;

	_pool_rand_maximum(pool, 32, maximum, mask, out, &err);

	return err;
}

static int _pool_rand40_maximum(struct pool *pool,
				uint64_t maximum, uint64_t mask, uint64_t *out)
{
	int err;

	_pool_rand_maximum(pool, 40, maximum, mask, out, &err);

	return err;
}

static int _pool_rand48_maximum(struct pool *pool,
				uint64_t maximum, uint64_t mask, uint64_t *out)
{
	int err;

	_pool_rand_maximum(pool, 48, maximum, mask, out, &err);

	return err;
}

static int _pool_rand56_maximum(struct pool *pool,
				uint64_t maximum, uint64_t mask, uint64_t *out)
{
	int err;

	_pool_rand_maximum(pool, 56, maximum, mask, out, &err);

	return err;
}

static int _pool_rand64_maximum(struct pool *pool,
				uint64_t maximum, uint64_t mask, uint64_t *out)
{
	int err;

	_pool_rand_maximum(pool, 64, maximum, mask, out, &err);

	return err;
}

static int pool_rand8_maximum(struct pool *pool,
			      uint8_t maximum, uint8_t *out)
{
	uint8_t mask = p2minus1(maximum);

	return _pool_rand8_maximum(pool, maximum, mask, out);
}

static int pool_rand16_maximum(struct pool *pool,
			       uint16_t maximum, uint16_t *out)
{
	unsigned int count;
	uint16_t mask;
	uint8_t u8;
	int err = -EINVAL;

	mask = p2minus1(maximum);
	count = (ilog2plus1(mask) + 7) / 8;

	switch (count) {
	case 0:
		*out = 0;
		err = 0;
		break;
	case 1:
		err = _pool_rand8_maximum(pool, maximum, mask, &u8);
		if (!err)
			*out = u8;
		break;
	case 2:
		err = _pool_rand16_maximum(pool, maximum, mask, out);
		break;
	}

	return err;
}

static int pool_rand32_maximum(struct pool *pool,
			       uint32_t maximum, uint32_t *out)
{
	unsigned int count;
	uint32_t mask;
	uint16_t u16;
	uint8_t u8;
	int err = -EINVAL;

	mask = p2minus1(maximum);
	count = (ilog2plus1(mask) + 7) / 8;

	switch (count) {
	case 0:
		*out = 0;
		err = 0;
		break;
	case 1:
		err = _pool_rand8_maximum(pool, maximum, mask, &u8);
		if (!err)
			*out = u8;
		break;
	case 2:
		err = _pool_rand16_maximum(pool, maximum, mask, &u16);
		if (!err)
			*out = u16;
		break;
	case 3:
		err = _pool_rand24_maximum(pool, maximum, mask, out);
		break;

	case 4:
		err = _pool_rand32_maximum(pool, maximum, mask, out);
		break;
	}

	return err;
}

static int pool_rand64_maximum(struct pool *pool,
			       uint64_t maximum, uint64_t *out)
{
	unsigned int count;
	uint64_t mask;
	uint32_t u32;
	uint16_t u16;
	uint8_t u8;
	int err = -EINVAL;

	mask = p2minus1(maximum);
	count = (ilog2plus1(mask) + 7) / 8;

	switch (count) {
	case 0:
		*out = 0;
		err = 0;
		break;
	case 1:
		err = _pool_rand8_maximum(pool, maximum, mask, &u8);
		if (!err)
			*out = u8;
		break;
	case 2:
		err = _pool_rand16_maximum(pool, maximum, mask, &u16);
		if (!err)
			*out = u16;
		break;
	case 3:
		err = _pool_rand24_maximum(pool, maximum, mask, &u32);
		if (!err)
			*out = u32;
		break;
	case 4:
		err = _pool_rand32_maximum(pool, maximum, mask, &u32);
		if (!err)
			*out = u32;
		break;
	case 5:
		err = _pool_rand40_maximum(pool, maximum, mask, out);
		break;
	case 6:
		err = _pool_rand48_maximum(pool, maximum, mask, out);
		break;
	case 7:
		err = _pool_rand56_maximum(pool, maximum, mask, out);
		break;
	case 8:
		err = _pool_rand64_maximum(pool, maximum, mask, out);
		break;
	}

	return err;
}

static int pool_rand_size_maximum(struct pool *pool,
				  size_t maximum, size_t *out)
{
	int err;

#if SIZE_MAX == UINT32_MAX
	uint32_t val;

	err = pool_rand32_maximum(pool, maximum, &val);

#elif SIZE_MAX == UINT64_MAX
	uint64_t val;

	err = pool_rand64_maximum(pool, maximum, &val);

#else
#error No known size for size_t
#endif

	if (err < 0)
		return err;

	*out = (size_t)val;

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

int futils_random_strong(void *buffer, size_t len)
{
	if (!buffer || len == 0)
		return -EINVAL;

	return rand_fetch(buffer, len);
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
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand8(pool, val);
}

int futils_random16(uint16_t *val)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand16(pool, val);
}

int futils_random32(uint32_t *val)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand32(pool, val);
}

int futils_random64(uint64_t *val)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand64(pool, val);
}

int futils_random8_maximum(uint8_t *val, uint8_t maximum)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand8_maximum(pool, maximum, val);
}

int futils_random16_maximum(uint16_t *val, uint16_t maximum)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand16_maximum(pool, maximum, val);
}

int futils_random32_maximum(uint32_t *val, uint32_t maximum)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand32_maximum(pool, maximum, val);
}

int futils_random64_maximum(uint64_t *val, uint64_t maximum)
{
	struct pool *pool = pool_get();

	if (!val)
		return -EINVAL;

	return pool_rand64_maximum(pool, maximum, val);
}

int futils_random_base16(void *buffer, size_t len, size_t count)
{
	/* catch int overflow (while not triggering size_t overflow) */
	ULOG_ERRNO_RETURN_ERR_IF(count > (((size_t)INT_MAX + 1) / 2), EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF((count * 2) > INT_MAX, EINVAL);

	static const char alphabet[] = "0123456789abcdef";

	struct pool *pool = pool_get();
	uint8_t *p = buffer;
	size_t ps = count * 2;

	if (!len)
		goto leave;

	if (ps > (len - 1))
		ps = (len - 1);

	while (ps >= 16) {
		uint64_t v;
		size_t i;
		int err;

		err = pool_rand64(pool, &v);
		if (err < 0)
			return err;

		for (i = 0; i < 16; i++) {
			*p = alphabet[v & 15];
			v >>= 4;
			p++;
			ps--;
		}
	}

	if (ps) {
		uint64_t v;
		size_t i;
		int err;

		err = pool_rand64(pool, &v);
		if (err < 0)
			return err;

		for (i = 0; i < ps; i++) {
			*p = alphabet[v & 15];
			v >>= 4;
			p++;
		}
		ps = 0;
	}

	/* NUL terminate string */
	*p = '\0';

leave:
	return count * 2;
}

int futils_random_base64(void *buffer, size_t len, size_t count)
{
	/* catch size_t overflow */
	ULOG_ERRNO_RETURN_ERR_IF(count > (SIZE_MAX - 2), EINVAL);
	/* catch int overflow (while not triggering size_t overflow) */
	ULOG_ERRNO_RETURN_ERR_IF((count + 2) / 3 > ((size_t)INT_MAX + 3) / 4,
				 EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(((count + 2) / 3) * 4 > INT_MAX, EINVAL);

	static const char alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"+/";

	struct pool *pool = pool_get();
	uint8_t *p = buffer;
	size_t ps = ((count + 2) / 3) * 4;

	size_t c = count;

	char b[4]; /* 4 * 6 bits for 3 * 8 bits */
	size_t bs;

	if (!len)
		goto leave;

	if (ps > (len - 1))
		ps = (len - 1);

	while (c >= 3 && ps) {

		uint32_t v;
		int err;

		err = pool_rand32(pool, &v);
		if (err < 0)
			return err;

		b[0] = alphabet[(v >>  0) & 63];
		b[1] = alphabet[(v >>  6) & 63];
		b[2] = alphabet[(v >> 12) & 63];
		b[3] = alphabet[(v >> 18) & 63];

		bs = 4;
		if (bs > ps)
			bs = ps;

		memcpy(p, b, bs);
		p += bs;
		ps -= bs;

		c -= 3;
	}

	/* last blob can end with one or two padding characters */
	if (c && ps) {

		uint32_t v;
		int err;

		err = pool_rand32(pool, &v);
		if (err < 0)
			return err;

		b[0] = alphabet[(v >> 0) & 63];
		b[1] = alphabet[(v >> 6) & 63];

		switch (c) {
		case 2:
			b[2] = alphabet[(v >> 12) & 63];
			break;
		case 1:
			b[2] = '=';
			break;
		}

		b[3] = '=';

		bs = 4;
		if (bs > ps)
			bs = ps;

		memcpy(p, b, bs);
		p += bs;
		ps -= bs;

		c = 0;
	}

	/* NUL terminate string */
	*p = '\0';

leave:
	return ((count + 2) / 3) * 4;
}

int futils_random_shuffle(void *base, size_t nmemb, size_t size)
{
	struct pool *pool = pool_get();
	uint8_t *p = base;
	size_t u;
	size_t v;
	size_t r;
	void *pu;
	void *pv;
	uint64_t tmpbuf;
	void *tmp = &tmpbuf;
	int err;

	ULOG_ERRNO_RETURN_ERR_IF(base == NULL, EINVAL);

	if (nmemb < 2)
		return 0;

	if (!size)
		return 0;

	if (size > sizeof(tmpbuf)) {
		tmp = malloc(size);
		if (!tmp)
			return -errno;
	}

	for (u = 0; u < nmemb - 1; u++) {

		r = 0;

		err = pool_rand_size_maximum(pool,
					     (nmemb - 1) - u, &r);
		if (err < 0)
			goto release;

		if (r == 0)
			continue;

		v = u + r;

		pu = p + u * size;
		pv = p + v * size;

		memcpy(tmp, pu, size);
		memcpy(pu, pv, size);
		memcpy(pv, tmp,  size);
	}

	err = 0;

release:
	if (tmp != &tmpbuf)
		free(tmp);

	return err;
}
