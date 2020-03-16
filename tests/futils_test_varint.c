/**
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
 * @file futils_test_varint.c
 *
 * @brief varint unit tests
 *
 */

#include "futils_test.h"

// varint 16
static const struct {
	uint16_t val;
	uint8_t data[3];
	size_t data_len;
} s_u16vals[] = {
	{0,		 {0x00, 0x00, 0x00}, 1},
	{1,		 {0x01, 0x00, 0x00}, 1},
	{UINT16_MAX - 1, {0xfe, 0xff, 0x03}, 3},
	{UINT16_MAX,	 {0xff, 0xff, 0x03}, 3},
};

static void test_varint_u16(void)
{
	int res;
	uint8_t data[3];
	size_t val_len;
	uint16_t uval;
	size_t i;

	// error test
	res = futils_varint_write_u16(NULL, sizeof(data),
				      s_u16vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_u16(data, 0,
				      s_u16vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_u16(data, sizeof(data),
				      s_u16vals[0].val, NULL);

	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u16(NULL, sizeof(s_u16vals[0].data),
				     &uval, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u16(s_u16vals[0].data, 0,
				     &uval, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u16(s_u16vals[0].data,
				     sizeof(s_u16vals[0].data),
				     NULL, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u16(s_u16vals[0].data,
				     sizeof(s_u16vals[0].data),
				     &uval, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	size_t vals_cnt = SIZEOF_ARRAY(s_u16vals);
	for (i = 0; i < vals_cnt; i++) {
		// write test
		res = futils_varint_write_u16(data, sizeof(data),
					      s_u16vals[i].val, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_u16vals[i].data_len);
		if (val_len == s_u16vals[i].data_len)
			CU_ASSERT_EQUAL(memcmp(s_u16vals[i].data, data, val_len), 0);



		// read test
		res = futils_varint_read_u16(s_u16vals[i].data,
					     sizeof(s_u16vals[i].data),
					     &uval, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_u16vals[i].data_len);
		CU_ASSERT_EQUAL(uval, s_u16vals[i].val);
	}
}

static const struct {
	int16_t val;
	uint8_t data[3];
	size_t data_len;
} s_i16vals[] = {
	{INT16_MIN,	{0xff, 0xff, 0x03}, 3},
	{INT16_MIN + 1, {0xfd, 0xff, 0x03}, 3},
	{-1, 		{0x01, 0x00, 0x00}, 1},
	{0, 		{0x00, 0x00, 0x00}, 1},
	{1, 		{0x02, 0x00, 0x00}, 1},
	{INT16_MAX - 1, {0xfc, 0xff, 0x03}, 3},
	{INT16_MAX,	{0xfe, 0xff, 0x03}, 3},
};

static void test_varint_i16(void)
{
	int res;
	uint8_t data[3];
	size_t val_len;
	int16_t ival;
	size_t i;

	// error test
	res = futils_varint_write_i16(NULL, sizeof(data),
				      s_i16vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_i16(data, 0,
				      s_i16vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_i16(data, sizeof(data),
				      s_i16vals[0].val, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	res = futils_varint_read_i16(NULL, sizeof(s_i16vals[i].data),
				     &ival, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i16(s_i16vals[0].data, 0,
				     &ival, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i16(s_i16vals[0].data,
				     sizeof(s_i16vals[0].data),
				     NULL, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i16(s_i16vals[0].data,
				     sizeof(s_i16vals[0].data),
				     &ival, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	size_t vals_cnt = SIZEOF_ARRAY(s_i16vals);
	for (i = 0; i < vals_cnt; i++) {
		// write test
		res = futils_varint_write_i16(data, sizeof(data),
					      s_i16vals[i].val, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_i16vals[i].data_len);
		if (val_len == s_i16vals[i].data_len)
			CU_ASSERT_EQUAL(memcmp(s_i16vals[i].data, data, val_len), 0);

		// read test
		res = futils_varint_read_i16(s_i16vals[i].data,
					     sizeof(s_i16vals[i].data),
					     &ival, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_i16vals[i].data_len);
		CU_ASSERT_EQUAL(ival, s_i16vals[i].val);
	}
}

// varint 32
static const struct {
	uint32_t val;
	uint8_t data[5];
	size_t data_len;
} s_u32vals[] = {
	{0,		 {0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{1,		 {0x01, 0x00, 0x00, 0x00, 0x00}, 1},
	{UINT32_MAX - 1, {0xfe, 0xff, 0xff, 0xff, 0x0f}, 5},
	{UINT32_MAX,	 {0xff, 0xff, 0xff, 0xff, 0x0f}, 5},
};

static void test_varint_u32(void)
{
	int res;
	uint8_t data[5];
	size_t val_len;
	uint32_t uval;
	size_t i;

	// error test
	res = futils_varint_write_u32(NULL, sizeof(data),
				      s_u32vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_u32(data, 0,
				      s_u32vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_u32(data, sizeof(data),
				      s_u32vals[0].val, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	res = futils_varint_read_u32(NULL, sizeof(s_u32vals[0].data),
				     &uval, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u32(s_u32vals[0].data, 0,
				     &uval, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u32(s_u32vals[0].data,
				     sizeof(s_u32vals[0].data),
				     NULL, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u32(s_u32vals[0].data,
				     sizeof(s_u32vals[0].data),
				     &uval, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	size_t vals_cnt = SIZEOF_ARRAY(s_u32vals);
	for (i = 0; i < vals_cnt; i++) {
		memset(data, 0, sizeof(data));

		// write test
		res = futils_varint_write_u32(data, sizeof(data),
					      s_u32vals[i].val, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_u32vals[i].data_len);
		if (val_len == s_u32vals[i].data_len)
			CU_ASSERT_EQUAL(memcmp(s_u32vals[i].data, data, val_len), 0);

		// read test
		res = futils_varint_read_u32(s_u32vals[i].data,
					     sizeof(s_u32vals[i].data),
					     &uval, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_u32vals[i].data_len);
		CU_ASSERT_EQUAL(uval, s_u32vals[i].val);
	}
}

static const struct {
	int32_t val;
	uint8_t data[5];
	size_t data_len;
} s_i32vals[] = {
	{INT32_MIN,	{0xff, 0xff, 0xff, 0xff, 0x0f}, 5},
	{INT32_MIN + 1, {0xfd, 0xff, 0xff, 0xff, 0x0f}, 5},
	{-1, 		{0x01, 0x00, 0x00, 0x00, 0x00}, 1},
	{0, 		{0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{1, 		{0x02, 0x00, 0x00, 0x00, 0x00}, 1},
	{INT32_MAX - 1, {0xfc, 0xff, 0xff, 0xff, 0x0f}, 5},
	{INT32_MAX,	{0xfe, 0xff, 0xff, 0xff, 0x0f}, 5},
};

static void test_varint_i32(void)
{
	int res;
	uint8_t data[5];
	size_t val_len;
	int32_t ival;
	size_t i;

	// error test
	res = futils_varint_write_i32(NULL, sizeof(data),
				      s_i32vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_i32(data, 0,
				      s_i32vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_i32(data, sizeof(data),
				      s_i32vals[0].val, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	res = futils_varint_read_i32(NULL, sizeof(s_i32vals[0].data),
				     &ival, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i32(s_i32vals[0].data, 0,
				     &ival, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i32(s_i32vals[0].data,
				     sizeof(s_i32vals[0].data),
				     NULL, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i32(s_i32vals[0].data,
				     sizeof(s_i32vals[0].data),
				     &ival, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	size_t vals_cnt = SIZEOF_ARRAY(s_i32vals);
	for (i = 0; i < vals_cnt; i++) {
		// write test
		res = futils_varint_write_i32(data, sizeof(data),
					      s_i32vals[i].val, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_i32vals[i].data_len);
		if (val_len == s_i32vals[i].data_len)
			CU_ASSERT_EQUAL(memcmp(s_i32vals[i].data, data, val_len), 0);

		// read test
		res = futils_varint_read_i32(s_i32vals[i].data,
					     sizeof(s_i32vals[i].data),
					     &ival, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_i32vals[i].data_len);
		CU_ASSERT_EQUAL(ival, s_i32vals[i].val);
	}
}

// varint 64
static const struct {
	uint64_t val;
	uint8_t data[10];
	size_t data_len;
} s_u64vals[] = {
	{0,		 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{1,		 {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{UINT64_MAX - 1, {0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}, 10},
	{UINT64_MAX,	 {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}, 10},
};

static void test_varint_u64(void)
{
	int res;
	uint8_t data[10];
	size_t val_len;
	uint64_t uval;
	size_t i;

	// error test
	res = futils_varint_write_u64(NULL, sizeof(data),
				      s_u64vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_u64(data, 0,
				      s_u64vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_u64(data, sizeof(data),
				      s_u64vals[0].val, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	res = futils_varint_read_u64(NULL, sizeof(s_u64vals[0].data),
				     &uval, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u64(s_u64vals[0].data, 0,
				     &uval, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u64(s_u64vals[0].data,
				     sizeof(s_u64vals[0].data),
				     NULL, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_u64(s_u64vals[0].data,
				     sizeof(s_u64vals[0].data),
				     &uval, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	size_t vals_cnt = SIZEOF_ARRAY(s_u64vals);
	for (i = 0; i < vals_cnt; i++) {
		// write test
		res = futils_varint_write_u64(data, sizeof(data),
					      s_u64vals[i].val, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_u64vals[i].data_len);
		if (val_len == s_u64vals[i].data_len)
			CU_ASSERT_EQUAL(memcmp(s_u64vals[i].data, data, val_len), 0);

		// read test
		res = futils_varint_read_u64(s_u64vals[i].data,
					     sizeof(s_u64vals[i].data),
					     &uval, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_u64vals[i].data_len);
		CU_ASSERT_EQUAL(uval, s_u64vals[i].val);
	}
}

static const struct {
	int64_t val;
	uint8_t data[10];
	size_t data_len;
} s_i64vals[] = {
	{INT64_MIN,	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}, 10},
	{INT64_MIN + 1, {0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}, 10},
	{-1, 		{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{0, 		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{1, 		{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1},
	{INT64_MAX - 1, {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}, 10},
	{INT64_MAX,	{0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01}, 10},
};

static void test_varint_i64(void)
{
	int res;
	uint8_t data[10];
	size_t val_len;
	int64_t ival;
	size_t i;

	// error test
	res = futils_varint_write_i64(NULL, sizeof(data),
				      s_i64vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_i64(data, 0,
				      s_i64vals[0].val, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_write_i64(data, sizeof(data),
				      s_i64vals[0].val, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	res = futils_varint_read_i64(NULL, sizeof(s_i64vals[0].data),
				     &ival, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i64(s_i64vals[0].data, 0,
				     &ival, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i64(s_i64vals[0].data,
				     sizeof(s_i64vals[0].data),
				     NULL, &val_len);
	CU_ASSERT_EQUAL(res, -EINVAL);
	res = futils_varint_read_i64(s_i64vals[0].data,
				     sizeof(s_i64vals[0].data),
				     &ival, NULL);
	CU_ASSERT_EQUAL(res, -EINVAL);

	size_t vals_cnt = SIZEOF_ARRAY(s_i64vals);
	for (i = 0; i < vals_cnt; i++) {
		// write test
		res = futils_varint_write_i64(data, sizeof(data),
					      s_i64vals[i].val, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_i64vals[i].data_len);
		if (val_len == s_i64vals[i].data_len)
			CU_ASSERT_EQUAL(memcmp(s_i64vals[i].data, data, val_len), 0);

		// read test
		res = futils_varint_read_i64(s_i64vals[i].data,
					     sizeof(s_i64vals[i].data),
					     &ival, &val_len);
		CU_ASSERT_EQUAL(res, 0);
		CU_ASSERT_EQUAL(val_len, s_i64vals[i].data_len);
		CU_ASSERT_EQUAL(ival, s_i64vals[i].val);
	}
}

CU_TestInfo s_varint_tests[] = {
	{(char *)"varint_u16", &test_varint_u16},
	{(char *)"varint_i16", &test_varint_i16},
	{(char *)"varint_u32", &test_varint_u32},
	{(char *)"varint_i32", &test_varint_i32},
	{(char *)"varint_u64", &test_varint_u64},
	{(char *)"varint_i64", &test_varint_i64},
};
