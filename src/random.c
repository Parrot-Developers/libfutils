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

#include "futils/random.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ULOG_TAG futils_random
#include <ulog.h>
ULOG_DECLARE_TAG(futils_random);

int futils_random_bytes(void *buffer, size_t len)
{
#ifdef _WIN32
	return -ENOSYS;
#else
	int fd;
	ssize_t rd;
	int ret = 0;

	if (!buffer || len == 0)
		return -EINVAL;

	fd = open("/dev/urandom", O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		ret = -errno;
		ULOG_ERRNO("open(/dev/urandom)", -ret);
		return ret;
	}

	rd = read(fd, buffer, len);
	if (rd < 0) {
		ret = -errno;
		ULOG_ERRNO("read", -ret);
	} else if ((size_t)rd != len) {
		ret = -EAGAIN;
		ULOGE("random_bytes: "
		      "not enough data to fill the buffer (%zd out of %zu)",
		      rd,
		      len);
	}

	close(fd);
	return ret;
#endif
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
