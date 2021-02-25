/**
 * Copyright (c) 2017 Parrot S.A.
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
 * @file futils_tests.c
 *
 * @brief libfutils unit tests.
 *
 */

#include "futils_test.h"
#include "futils/futils.h"
#include "stdlib.h"

extern CU_TestInfo s_mbox_tests[];
extern CU_TestInfo s_dynmbox_tests[];
extern CU_TestInfo s_systimetools_tests[];
extern CU_TestInfo s_list_tests[];
extern CU_TestInfo s_random_tests[];
extern CU_TestInfo s_varint_tests[];
extern CU_TestInfo s_timetools_tests[];
extern CU_TestInfo s_safew_tests[];

static void test_bound(void)
{
	uint32_t value = 10;
	uint32_t min = 5;
	uint32_t max = 20;
	CU_ASSERT_EQUAL(value, BOUND(value, min, max));

	value = 0;
	CU_ASSERT_EQUAL(min, BOUND(value, min, max));

	value = 30;
	CU_ASSERT_EQUAL(max, BOUND(value, min, max));
}

CU_TestInfo s_futils_tests[] = {
	{(char *)"futils bound", &test_bound},
	CU_TEST_INFO_NULL,
};

static CU_SuiteInfo s_suites[] = {
	{
		.pName = (char *)"futils",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_futils_tests
	},
	{
		.pName = (char *)"mbox",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_mbox_tests
	},
	{
		.pName = (char *)"dynmbox",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_dynmbox_tests
	},
	{
		.pName = (char *)"systimetools",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_systimetools_tests
	},
	{
		.pName = (char *)"list",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_list_tests
	},
	{
		.pName = (char *)"random",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_random_tests
	},
	{
		.pName = (char *)"varint",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_varint_tests
	},
	{
		.pName = (char *)"timetools",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_timetools_tests
	},
	{
		.pName = (char *)"safew",
		.pInitFunc = NULL,
		.pCleanupFunc = NULL,
		.pTests = s_safew_tests
	},
	CU_SUITE_INFO_NULL,
};

int main(void)
{
	CU_initialize_registry();
	CU_register_suites(s_suites);
	if (getenv("CUNIT_OUT_NAME") != NULL)
		CU_set_output_filename(getenv("CUNIT_OUT_NAME"));
	if (getenv("CUNIT_AUTOMATED") != NULL) {
		CU_automated_run_tests();
		CU_list_tests_to_file();
	} else {
		CU_basic_set_mode(CU_BRM_VERBOSE);
		CU_basic_run_tests();
	}
	CU_cleanup_registry();
	return 0;
}
