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
 * @file futils_tests_string.cpp
 *
 * @brief libfutils cpp string unit tests.
 *
 */

#include "futils_test.h"
#include <string>

struct test_prefix {
	std::string input;
	std::string prefix;
	bool expected_check;
};

struct test_suffix {
	std::string input;
	std::string suffix;
	bool expected_check;
};

static struct test_prefix test_prefix_check_data[] = {
	{
		.input = "qwerty",
		.prefix = "qw",
		.expected_check = true,
	},
	{
		.input = "qwerty",
		.prefix = "ty",
		.expected_check = false,
	},
	{
		.input = "qwerty",
		.prefix = "er",
		.expected_check = false,
	},
	{
		.input = "qwerty",
		.prefix = "",
		.expected_check = true,
	},
	{
		.input = "",
		.prefix = "qwerty",
		.expected_check = false,
	},
	{
		.input = "qwerty",
		.prefix = "qwerty",
		.expected_check = true,
	},
	{
		.input = "qwerty",
		.prefix = "qwqwerty",
		.expected_check = false,
	},
	{
		.input = "",
		.prefix = "",
		.expected_check = true,
	},
};

static struct test_suffix test_suffix_check_data[] = {
	{
		.input = "qwerty",
		.suffix = "ty",
		.expected_check = true,
	},
	{
		.input = "qwerty",
		.suffix = "qw",
		.expected_check = false,
	},
	{
		.input = "qwerty",
		.suffix = "er",
		.expected_check = false,
	},
	{
		.input = "qwerty",
		.suffix = "",
		.expected_check = true,
	},
	{
		.input = "",
		.suffix = "qwerty",
		.expected_check = false,
	},
	{
		.input = "qwerty",
		.suffix = "qwerty",
		.expected_check = true,
	},
	{
		.input = "qwerty",
		.suffix = "qwertyty",
		.expected_check = false,
	},
	{
		.input = "",
		.suffix = "",
		.expected_check = true,
	},
};

static void test_string_prefix(void)
{
	size_t i;
	for (i = 0; i < SIZEOF_ARRAY(test_prefix_check_data); i++) {
		struct test_prefix &test = test_prefix_check_data[i];
		bool ret = futils::string::startsWith(test.input, test.prefix);
		CU_ASSERT_EQUAL(ret, test.expected_check);
	}
}

static void test_string_suffix(void)
{
	size_t i;
	for (i = 0; i < SIZEOF_ARRAY(test_suffix_check_data); i++) {
		struct test_suffix &test = test_suffix_check_data[i];
		bool ret = futils::string::endsWith(test.input, test.suffix);
		CU_ASSERT_EQUAL(ret, test.expected_check);
	}
}

CU_TestInfo s_string_cpp_tests[] = {
	{(char *)"prefix", &test_string_prefix},
	{(char *)"suffix", &test_string_suffix},
	CU_TEST_INFO_NULL,
};
