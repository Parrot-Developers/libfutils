/**
 * Copyright (c) 2022 Parrot S.A.
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
 * @file futils_tests_string.c
 *
 * @brief libfutils string unit tests.
 *
 */

#include "futils_test.h"
#include <errno.h>

struct test_sanitize_string {
	/* Input string */
	const char *raw_input;
	/* Forbidden chars */
	const wchar_t *forbidden;
	wchar_t replacement;
	/* sanitize params */
	size_t max_len;
	const char *expected;
};

struct test_check_string {
	/* Input string */
	const char *raw_input;
	/* Forbidden chars */
	const wchar_t *forbidden;
	/* check params */
	size_t max_bytes;
	size_t max_wchars;
	int expected_check;
};

static struct test_sanitize_string test_sanitize_data[] = {
	/* ASCII string, good length, no forbidden */
	{
		.raw_input = "azerty",
		.forbidden = NULL,
		.replacement = L'_',
		.max_len = 15,
		.expected = "azerty",
	},
	/* ASCII string, good length, forbidden chars not in string */
	{
		.raw_input = "azerty",
		.forbidden = L"uiop",
		.replacement = L'_',
		.max_len = 15,
		.expected = "azerty",
	},
	/* ASCII string, too long, no forbidden */
	{
		.raw_input = "azertyuiop",
		.forbidden = NULL,
		.replacement = L'_',
		.max_len = 7,
		.expected = "azerty",
	},
	/* ASCII string, good length, forbidden chars */
	{
		.raw_input = "azerty",
		.forbidden = L"zqs",
		.replacement = L'_',
		.max_len = 15,
		.expected = "a_erty",
	},
	/* ASCII string, too long, forbidden chars */
	{
		.raw_input = "azertyuiop",
		.forbidden = L"zrqs",
		.replacement = L'_',
		.max_len = 7,
		.expected = "a_e_ty",
	},
	/* UTF-8 tests uses japanese kanas, which are 3 bytes long each */
	/* UTF-8 string, good length, no forbidden */
	{
		.raw_input = "フチルス",
		.forbidden = NULL,
		.replacement = L'_',
		.max_len = 15,
		.expected = "フチルス",
	},
	/* UTF-8 string, good length, forbidden chars not in string */
	{
		.raw_input = "フチルス",
		.forbidden = L"イカシキ",
		.replacement = L'_',
		.max_len = 15,
		.expected = "フチルス",
	},
	/* UTF-8 string, too long, no forbidden */
	{
		.raw_input = "フチルス",
		.forbidden = NULL,
		.replacement = L'_',
		.max_len = 7,
		.expected = "フチ",
	},
	/* UTF-8 string, good length, forbidden chars */
	{
		.raw_input = "フチルス",
		.forbidden = L"ルタ",
		.replacement = L'_',
		.max_len = 15,
		.expected = "フチ_ス",
	},
	/* UTF-8 string, too long, forbidden chars */
	{
		.raw_input = "フチルス",
		.forbidden = L"フチタ",
		.replacement = L'_',
		.max_len = 6,
		.expected = "__ル",
	},
};

static struct test_check_string test_check_data[] = {
	/* ASCII string, good length, no forbidden */
	{
		.raw_input = "azerty",
		.forbidden = NULL,
		.max_bytes = 7,
		.max_wchars = 6,
		.expected_check = 0,
	},
	/* ASCII string, good length, forbidden chars not in string */
	{
		.raw_input = "azerty",
		.forbidden = L"uiop",
		.max_bytes = 7,
		.max_wchars = 6,
		.expected_check = 0,
	},
	/* ASCII string, too long (bytes), no forbidden */
	{
		.raw_input = "azerty",
		.forbidden = NULL,
		.max_bytes = 3,
		.max_wchars = 6,
		.expected_check = -E2BIG,
	},
	/* ASCII string, too long (wchar), no forbidden */
	{
		.raw_input = "azerty",
		.forbidden = NULL,
		.max_bytes = 7,
		.max_wchars = 3,
		.expected_check = -E2BIG,
	},
	/* ASCII string, good size, forbidden chars */
	{
		.raw_input = "azerty",
		.forbidden = L"z",
		.max_bytes = 7,
		.max_wchars = 6,
		.expected_check = -EPROTO,
	},
	/* ASCII string, too long (bytes & wchar), forbidden chars */
	{
		.raw_input = "azerty",
		.forbidden = L"z",
		.max_bytes = 3,
		.max_wchars = 3,
		.expected_check = -E2BIG,
	},
	/* UTF-8 tests uses japanese kanas, which are 3 bytes long each */
	/* UTF-8 string, good length, no forbidden */
	{
		.raw_input = "フチルス",
		.forbidden = NULL,
		.max_bytes = 13,
		.max_wchars = 4,
		.expected_check = 0,
	},
	/* UTF-8 string, good length, forbidden chars not in string */
	{
		.raw_input = "フチルス",
		.forbidden = L"イカシキ",
		.max_bytes = 13,
		.max_wchars = 4,
		.expected_check = 0,
	},
	/* UTF-8 string, too long (bytes), no forbidden */
	{
		.raw_input = "フチルス",
		.forbidden = NULL,
		.max_bytes = 8,
		.max_wchars = 4,
		.expected_check = -E2BIG,
	},
	/* UTF-8 string, too long (wchar), no forbidden */
	{
		.raw_input = "フチルス",
		.forbidden = NULL,
		.max_bytes = 13,
		.max_wchars = 3,
		.expected_check = -E2BIG,
	},
	/* UTF-8 string,  good size, forbidden chars */
	{
		.raw_input = "フチルス",
		.forbidden = L"フ",
		.max_bytes = 13,
		.max_wchars = 4,
		.expected_check = -EPROTO,
	},
	/* UTF-8 string, too long (bytes & wchar), forbidden chars */
	{
		.raw_input = "フチルス",
		.forbidden = L"チ",
		.max_bytes = 8,
		.max_wchars = 3,
		.expected_check = -E2BIG,
	},
};


static void test_string_sanitize(void)
{
	size_t i;
	for (i = 0; i < SIZEOF_ARRAY(test_sanitize_data); i++) {
		struct test_sanitize_string *test = &test_sanitize_data[i];
		char output[test->max_len];
		int ret = futils_string_sanitize_utf8(test->raw_input,
						      output,
						      test->max_len,
						      test->forbidden,
						      test->replacement);
		CU_ASSERT_EQUAL(ret, strlen(test->expected));
		CU_ASSERT_STRING_EQUAL(output, test->expected);
	}
}

static void test_string_check(void)
{
	size_t i;
	for (i = 0; i < SIZEOF_ARRAY(test_check_data); i++) {
		struct test_check_string *test = &test_check_data[i];
		int ret = futils_string_check_utf8(test->raw_input,
						   test->max_bytes,
						   test->max_wchars,
						   test->forbidden);
		CU_ASSERT_EQUAL(ret, test->expected_check);
	}
}

CU_TestInfo s_string_tests[] = {
	{(char *)"sanitize", &test_string_sanitize},
	{(char *)"check", &test_string_check},
	CU_TEST_INFO_NULL,
};
