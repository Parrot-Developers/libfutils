/*
 * Copyright (c) 2020 Parrot S.A.
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
 * @file futils_random.c
 *
 * @brief generate random bytes (and report speed)
 *
 */

#include "futils/random.h"
#include "futils/timetools.h"
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GB (1024 * 1024 * 1024)
#define MB (1024 * 1024)
#define KB (1024)

/* codecheck_ignore[VOLATILE] */
static volatile sig_atomic_t running = 1;

static void sighandler(int sig)
{
	/* allows up to 2 signals to be received before
	   the program will be abruptly terminated */
	static sig_atomic_t ignored = 2;

	if (!ignored) {
		signal(sig, SIG_DFL);
		if (raise(sig) != 0)
			abort();
		_Exit(EXIT_FAILURE);
	}

	ignored--;

	running = 0;
}

static int test_futils_random8(void *buffer, size_t size, size_t *ret)
{
	uint8_t *p = buffer;
	size_t s = size;
	int err;

	while (s && running) {

		err = futils_random8(p);
		if (err < 0)
			return err;

		s--;
		p++;
	}

	*ret = size - s;

	return 0;
}

static int test_futils_random16(void *buffer, size_t size, size_t *ret)
{
	uint16_t *p = buffer;
	size_t s = size;
	int err;

	while (s >= sizeof(uint16_t) && running) {

		err = futils_random16(p);
		if (err < 0)
			return err;

		s -= sizeof(uint16_t);
		p++;
	}

	if (s && running) {

		uint16_t tmp;

		err = futils_random16(&tmp);
		if (err < 0)
			return err;

		memcpy(p, &tmp, s);

		s = 0;
	}

	*ret = size - s;

	return 0;
}

static int test_futils_random32(void *buffer, size_t size, size_t *ret)
{
	uint32_t *p = buffer;
	size_t s = size;
	int err;

	while (s >= sizeof(uint32_t) && running) {

		err = futils_random32(p);
		if (err < 0)
			return err;

		s -= sizeof(uint32_t);
		p++;
	}

	if (s && running) {

		uint32_t tmp;

		err = futils_random32(&tmp);
		if (err < 0)
			return err;

		memcpy(p, &tmp, s);

		s = 0;
	}

	*ret = size - s;

	return 0;
}

static int test_futils_random64(void *buffer, size_t size, size_t *ret)
{
	uint64_t *p = buffer;
	size_t s = size;
	int err;

	while (s >= sizeof(uint64_t) && running) {

		err = futils_random64(p);
		if (err < 0)
			return err;

		s -= sizeof(uint64_t);
		p++;
	}

	if (s && running) {

		uint64_t tmp;

		err = futils_random64(&tmp);
		if (err < 0)
			return err;

		memcpy(p, &tmp, s);

		s = 0;
	}

	*ret = size - s;

	return 0;
}

static int test_futils_random_bytes(void *buffer, size_t size, size_t *ret)
{
	int err;

	err = futils_random_bytes(buffer, size);
	if (err < 0)
		return err;

	*ret = size;

	return 0;
}

static int test_memset(void *buffer, size_t size, size_t *ret)
{
	/* baseline: only write */
	memset(buffer, 0, size);

#if defined(__GNUC__)
	/* pretend the content of buffer might be modified */
	__asm__ __volatile__("" : : "r"(buffer) : "memory");
#endif

	*ret = size;

	return 0;
}

static int test_noop(void *buffer, size_t size, size_t *ret)
{
	/* baseline: noop */

#if defined(__GNUC__)
	/* pretend something happen here */
	__asm__ __volatile__("");
#endif

	*ret = size;

	return 0;
}

int main(int argc, char *argv[])
{
	bool unlimited = true;
	uintmax_t limit;
	uintmax_t remaining = 0;
	struct timespec start;
	struct timespec duration;
	uint64_t duration_ns;
	uintmax_t total_ns = 0;
	uintmax_t total_bytes = 0;
	void *buffer;
	size_t buffer_size;
	const char *test_name;
	int (*test)(void *, size_t, size_t *);

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <test> [bytes]\n", argv[0]);
		return EXIT_FAILURE;
	}

	test_name = argv[1];

	if (strcmp(test_name, "random8") == 0)
		test = test_futils_random8;
	else if (strcmp(test_name, "random16") == 0)
		test = test_futils_random16;
	else if (strcmp(test_name, "random32") == 0)
		test = test_futils_random32;
	else if (strcmp(test_name, "random64") == 0)
		test = test_futils_random64;
	else if (strcmp(test_name, "random_bytes") == 0 ||
		 strcmp(test_name, "random-bytes") == 0)
		test = test_futils_random_bytes;
	else if (strcmp(test_name, "memset") == 0)
		test = test_memset;
	else if (strcmp(test_name, "noop") == 0)
		test = test_noop;
	else {
		fprintf(stderr, "%s: '%s': unknown test\n", argv[0], test_name);
		return EXIT_FAILURE;
	}

	if (argc == 3) {
		const char *limit_str = argv[2];
		char *p = NULL;
		errno = 0;
		limit = strtoumax(limit_str, &p, 0);
		if (limit == 0 || limit == UINTMAX_MAX) {
			int err = errno;
			if (err) {
				fprintf(stderr,
					"%s: '%s': strtoul() failed with error %d (%s)\n",
					argv[0], limit_str, err, strerror(err));
				return EXIT_FAILURE;
			}
		}

		if (p) {
			if (*p) {
				fprintf(stderr, "%s: '%s': malformed number\n",
					argv[0], limit_str);
				return EXIT_FAILURE;
			}

			if (p == limit_str) {
				fprintf(stderr, "%s: '%s': empty string\n",
					argv[0], limit_str);
				return EXIT_FAILURE;
			}
		}

		remaining = limit;
		unlimited = false;
	}

	buffer_size = 64 * MB;

	if (!unlimited)
		if (buffer_size > limit)
			buffer_size = limit;

	buffer = malloc(buffer_size);
	if (!buffer) {
		int err = errno;
		fprintf(stderr, "%s: malloc() failed: %d (%s)\n",
			argv[0], err, strerror(err));
		return EXIT_FAILURE;
	}

	/* warm up the buffer */
	memset(buffer, 0, buffer_size);

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	while (running) {

		size_t requested = buffer_size;
		size_t generated;
		int err;

		if (!unlimited) {
			if (!remaining)
				break;

			if (requested > remaining)
				requested = remaining;
		}

		time_get_monotonic(&start);

		err = test(buffer, requested, &generated);
		if (err < 0) {
			fprintf(stderr, "%s: test %s failed: %d (%s)\n",
				argv[0], test_name, -err, strerror(-err));
			return EXIT_FAILURE;
		}

		time_timespec_diff_now(&start, &duration);

		time_timespec_to_ns(&duration, &duration_ns);

		total_ns += duration_ns;
		total_bytes += generated;

		if (!unlimited)
			remaining -= generated;

		fwrite(buffer, generated, 1, stdout);
	}

	fprintf(stderr,	"speed = ");

	if (total_ns) {
		double speed;

		speed = 1000000000.0 * total_bytes;
		speed /= total_ns;

		if ((speed / GB) >= 1.0)
			fprintf(stderr,
				"%.3f GiB/s",
				speed / GB);
		else if ((speed / MB) >= 1.0)
			fprintf(stderr,
				"%.3f MiB/s",
				speed / MB);
		else if ((speed / KB) >= 1.0)
			fprintf(stderr,
				"%.3f KiB/s",
				speed / KB);
		else
			fprintf(stderr,
				"%.3f B/s",
				speed);
	} else
		fprintf(stderr,
			"undefined");

	fprintf(stderr,
		" (%ju bytes in %ju ns)\n",
		total_bytes, total_ns);

	return EXIT_SUCCESS;
}
