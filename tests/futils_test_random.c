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
 * @file futils_test_random.c
 *
 * @brief random unit tests
 *
 */

#include "futils_test.h"

static void test_random8_maximum(void)
{
	uint8_t maximum;
	uint8_t val;
	int err;

	/* test 0b0, 0b1, 0b11, 0b111, ... maximum
	   and check that output bits matches */
	maximum = 0;

	while (1) {
		uint8_t mask = 0;
		unsigned int i;

		for (i = 0; i < 1024; i++) {
			err = futils_random8_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			mask |= val;

			if (mask >= maximum)
				break;
		}

		CU_ASSERT_EQUAL_FATAL(mask, maximum);

		if (maximum == UINT8_MAX)
			break;

		maximum <<= 1;
		maximum |= 1;
	}

	/* test all maximums, from 0 to 255,
	   get some output values and ensure
	   they're no bigger than maximum */
	err = futils_random8_maximum(&val, 0);
	CU_ASSERT_EQUAL_FATAL(err, 0);
	CU_ASSERT_EQUAL(val, 0);

	for (maximum = 1; maximum; maximum++) {
		uint8_t i;

		for (i = 0; i < maximum; i++) {
			err = futils_random8_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			CU_ASSERT_TRUE(val <= maximum);
		}
	}
}

static const uint16_t maximum16[] = {
	0, 1,
	(UINT16_C(1) <<  8) - 1, (UINT16_C(1) <<  8), (UINT16_C(1) <<  8) + 1,
	UINT16_MAX
};

static void test_random16_maximum(void)
{
	uint16_t maximum;
	uint16_t val;
	size_t i;
	int err;

	/* test 0b0, 0b1, 0b11, 0b111, ... maximum
	   and check that output bits matches */
	maximum = 0;

	while (1) {
		uint16_t mask = 0;

		for (i = 0; i < 2048; i++) {
			err = futils_random16_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			mask |= val;

			if (mask >= maximum)
				break;
		}

		CU_ASSERT_EQUAL_FATAL(mask, maximum);

		if (maximum == UINT16_MAX)
			break;

		maximum <<= 1;
		maximum |= 1;
	}

	/* test some interesting maximums,
	   get some output values and ensure
	   they're no bigger than maximum */
	for (i = 0; i < SIZEOF_ARRAY(maximum16); i++) {
		unsigned int j;

		maximum = maximum16[i];

		for (j = 0; j < 1024; j++) {
			err = futils_random16_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			CU_ASSERT_TRUE(val <= maximum);
		}
	}
}

static const uint32_t maximum32[] = {
	0, 1,
	(UINT32_C(1) <<  8) - 1, (UINT32_C(1) <<  8), (UINT32_C(1) <<  8) + 1,
	(UINT32_C(1) << 16) - 1, (UINT32_C(1) << 16), (UINT32_C(1) << 16) + 1,
	(UINT32_C(1) << 24) - 1, (UINT32_C(1) << 24), (UINT32_C(1) << 24) + 1,
	UINT32_MAX
};

static void test_random32_maximum(void)
{
	uint32_t maximum;
	uint32_t val;
	size_t i;
	int err;

	/* test 0b0, 0b1, 0b11, 0b111, ... maximum
	   and check that output bits matches */
	maximum = 0;

	while (1) {
		uint32_t mask = 0;

		for (i = 0; i < 4096; i++) {
			err = futils_random32_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			mask |= val;

			if (mask >= maximum)
				break;
		}

		CU_ASSERT_EQUAL_FATAL(mask, maximum);

		if (maximum == UINT32_MAX)
			break;

		maximum <<= 1;
		maximum |= 1;
	}

	/* test some interesting maximums,
	   get some output values and ensure
	   they're no bigger than maximum */
	for (i = 0; i < SIZEOF_ARRAY(maximum32); i++) {
		unsigned int j;

		maximum = maximum32[i];

		for (j = 0; j < 1024; j++) {
			err = futils_random32_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			CU_ASSERT_TRUE(val <= maximum);
		}
	}
}

static const uint64_t maximum64[] = {
	0, 1,
	(UINT64_C(1) <<  8) - 1, (UINT64_C(1) <<  8), (UINT64_C(1) <<  8) + 1,
	(UINT64_C(1) << 16) - 1, (UINT64_C(1) << 16), (UINT64_C(1) << 16) + 1,
	(UINT64_C(1) << 24) - 1, (UINT64_C(1) << 24), (UINT64_C(1) << 24) + 1,
	(UINT64_C(1) << 32) - 1, (UINT64_C(1) << 32), (UINT64_C(1) << 32) + 1,
	(UINT64_C(1) << 40) - 1, (UINT64_C(1) << 40), (UINT64_C(1) << 40) + 1,
	(UINT64_C(1) << 48) - 1, (UINT64_C(1) << 48), (UINT64_C(1) << 48) + 1,
	(UINT64_C(1) << 56) - 1, (UINT64_C(1) << 56), (UINT64_C(1) << 56) + 1,
	UINT64_MAX
};

static void test_random64_maximum(void)
{
	uint64_t maximum;
	uint64_t val;
	size_t i;
	int err;

	/* test 0b0, 0b1, 0b11, 0b111, ... maximum
	   and check that output bits match */
	maximum = 0;

	while (1) {
		uint64_t mask = 0;

		for (i = 0; i < 8192; i++) {
			err = futils_random64_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			mask |= val;

			if (mask >= maximum)
				break;
		}

		CU_ASSERT_EQUAL_FATAL(mask, maximum);

		if (maximum == UINT64_MAX)
			break;

		maximum <<= 1;
		maximum |= 1;
	}

	/* test some interesting maximums,
	   get some output values and ensure
	   they're no bigger than maximum */
	for (i = 0; i < SIZEOF_ARRAY(maximum64); i++) {
		unsigned int j;

		maximum = maximum64[i];

		for (j = 0; j < 1024; j++) {
			err = futils_random64_maximum(&val, maximum);
			CU_ASSERT_EQUAL_FATAL(err, 0);

			CU_ASSERT_TRUE(val <= maximum);
		}
	}
}

static void test_random_base16(void)
{
	/* count cannot be that big otherwise it would lead to int overflow */
	const size_t count_max = ((size_t)INT_MAX + 1) / 2;

	CU_ASSERT_EQUAL(futils_random_base16(NULL, 0, count_max - 1),
			(count_max - 1) * 2);

	CU_ASSERT_EQUAL(futils_random_base16(NULL, 0, count_max),
			-EINVAL);

	for (size_t len = 0; len < 256; len++)
		for (size_t count = 0; count < 256; count++) {

			int actual_ret;
			size_t expected_ret = count * 2;

			/* allocate a new buffer to let ASan / Valgrind catch
			   out of bound access / read to undefined */
			void *buffer = malloc(len);
			CU_ASSERT_PTR_NOT_NULL_FATAL(buffer);

			actual_ret = futils_random_base16(buffer, len, count);
			CU_ASSERT_TRUE_FATAL(actual_ret >= 0);

			CU_ASSERT_EQUAL((size_t)actual_ret, expected_ret);

			if (len) {
				if (expected_ret > (len - 1)) {
					CU_ASSERT_EQUAL(strlen(buffer),
							len - 1);
				} else {
					CU_ASSERT_EQUAL(strlen(buffer),
							expected_ret);
				}
			}

			free(buffer);
		}
}

CU_TestInfo s_random_tests[] = {
	{(char *)"random8 maximum", &test_random8_maximum},
	{(char *)"random16 maximum", &test_random16_maximum},
	{(char *)"random32 maximum", &test_random32_maximum},
	{(char *)"random64 maximum", &test_random64_maximum},
	{(char *)"random base16", &test_random_base16},
	CU_TEST_INFO_NULL,
};
