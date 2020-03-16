/******************************************************************************
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
 * @file varint.c
 *
 * @brief variable-length quantity (VLQ) API
 *        https://en.wikipedia.org/wiki/Variable-length_quantity
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "futils/varint.h"

/**
 * Reads a variable unsigned integer from data.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 * @param val_len_max : Maximum value length in byte read.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
static int futils_varint_read(const uint8_t *src, size_t src_len,
		uint64_t *val, size_t *val_len, size_t val_len_max)
{
	uint32_t shift = 0;
	size_t offset = 0;
	int more = 0;

	if (src == NULL || src_len == 0 || val == NULL || val_len == NULL)
		return -EINVAL;

	*val = 0;
	/* Decode value */
	do {
		*val |= (((uint64_t)(src[offset] & 0x7f)) << shift);
		more = src[offset] & 0x80;

		offset++;
		shift += 7;
		if ((offset > src_len) ||
		    (offset > val_len_max) ||
		    (offset == val_len_max && src[offset] & 0x0f))
			return -EPROTO;
	} while (more);

	*val_len = offset;
	return 0;
}

/**
 * Writes a variable unsigned integer in data.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ;
 *                  Should be greater or equal to `val_len_max`.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 * @param val_len_max : Maximum value length in byte read.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
static int futils_varint_write(uint8_t *dst, size_t dst_len,
		uint64_t val, size_t *val_len, size_t val_len_max)
{
	size_t off = 0;
	uint8_t byte = 0;
	int more = 0;

	if (dst == NULL || dst_len < val_len_max || val_len == NULL)
		return -EINVAL;

	/* Process value, use logical right shift without sign propagation */
	do {
		byte = val & 0x7f;
		val >>= 7;
		more = (val != 0);
		if (more)
			byte |= 0x80;
		dst[off++] = byte;
	} while (more);

	*val_len = off;
	return 0;
}

int futils_varint_read_u16(const uint8_t *src, size_t src_len,
		uint16_t *val, size_t *val_len)
{
	uint64_t val64;
	int res;

	if (val == NULL)
		return -EINVAL;

	res = futils_varint_read(src, src_len,
			&val64, val_len, sizeof(*val) + 1);

	*val = (uint16_t)val64;
	return res;
}

int futils_varint_write_u16(uint8_t *dst, size_t dst_len,
		uint16_t val, size_t *val_len)
{
	return futils_varint_write(dst, dst_len, val, val_len, sizeof(val) + 1);
}

int futils_varint_read_i16(const uint8_t *src, size_t src_len,
		int16_t *val, size_t *val_len)
{
	uint64_t val64;
	int res;

	if (val == NULL)
		return -EINVAL;

	res = futils_varint_read(src, src_len,
			&val64, val_len, sizeof(*val) + 1);

	/* Zigzag decoding, use logical right shift, without sign propagation */
	*val = ((int16_t)(val64 >> 1)) ^ -((int16_t)(val64 & 0x1));
	return res;
}

int futils_varint_write_i16(uint8_t *dst, size_t dst_len,
		int16_t val, size_t *val_len)
{
	/* Zigzag encoding, use arithmetic right shift, with sign propagation */
	uint64_t val64 = (uint16_t)((val << 1) ^ (val >> 15));
	return futils_varint_write(dst, dst_len,
			val64, val_len, sizeof(val) + 1);
}

int futils_varint_read_u32(const uint8_t *src, size_t src_len,
		uint32_t *val, size_t *val_len)
{
	uint64_t val64;
	int res;

	if (val == NULL)
		return -EINVAL;

	res = futils_varint_read(src, src_len,
			&val64, val_len, sizeof(*val) + 1);

	*val = (uint32_t)val64;
	return res;
}

int futils_varint_write_u32(uint8_t *dst, size_t dst_len,
		uint32_t val, size_t *val_len)
{
	return futils_varint_write(dst, dst_len, val, val_len, sizeof(val) + 1);
}

int futils_varint_read_i32(const uint8_t *src, size_t src_len,
		int32_t *val, size_t *val_len)
{
	uint64_t val64;
	int res;

	if (val == NULL)
		return -EINVAL;

	res = futils_varint_read(src, src_len,
			&val64, val_len, sizeof(*val) + 1);

	/* Zigzag decoding, use logical right shift, without sign propagation */
	*val = ((int32_t)(val64 >> 1)) ^ -((int32_t)(val64 & 0x1));
	return res;
}

int futils_varint_write_i32(uint8_t *dst, size_t dst_len,
		int32_t val, size_t *val_len)
{
	/* Zigzag encoding, use arithmetic right shift, with sign propagation */
	uint64_t val64 = (uint32_t)((val << 1) ^ (val >> 31));
	return futils_varint_write(dst, dst_len,
			val64, val_len, sizeof(val) + 1);
}

int futils_varint_read_u64(const uint8_t *src, size_t src_len,
		uint64_t *val, size_t *val_len)
{
	return futils_varint_read(src, src_len,
			val, val_len, sizeof(*val) + 2);
}

int futils_varint_write_u64(uint8_t *dst, size_t dst_len,
		uint64_t val, size_t *val_len)
{
	return futils_varint_write(dst, dst_len, val, val_len, sizeof(val) + 2);
}

int futils_varint_read_i64(const uint8_t *src, size_t src_len,
		int64_t *val, size_t *val_len)
{
	uint64_t val64;
	int res;

	if (val == NULL)
		return -EINVAL;

	res = futils_varint_read(src, src_len,
			&val64, val_len, sizeof(*val) + 2);

	/* Zigzag decoding, use logical right shift, without sign propagation */
	*val = ((int64_t)(val64 >> 1)) ^ -((int64_t)(val64 & 0x1));
	return res;
}

int futils_varint_write_i64(uint8_t *dst, size_t dst_len,
		int64_t val, size_t *val_len)
{
	/* Zigzag encoding, use arithmetic right shift, with sign propagation */
	uint64_t val64 = (uint64_t)((val << 1) ^ (val >> 63));
	return futils_varint_write(dst, dst_len,
			val64, val_len, sizeof(val) + 2);
}
