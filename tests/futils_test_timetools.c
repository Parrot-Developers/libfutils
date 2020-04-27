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
 * @file futils_test_timetools.c
 *
 * @brief timetools unit tests.
 *
 */

#include "futils_test.h"

static void test_timetools_monotonic(void)
{
	struct timespec ts_start;
	struct timespec ts_end;
	uint64_t start;
	uint64_t end;
	int64_t delta;
	int rc;

	rc = time_get_monotonic(&ts_start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ns(&ts_start, &start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_get_monotonic(&ts_end);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ns(&ts_end, &end);
	CU_ASSERT_EQUAL(rc, 0);

	delta = end - start;

	CU_ASSERT(delta >= 0);
}

static void test_timetools_cmp(void)
{
	struct timespec ts_0;
	struct timespec ts_1;
	struct timespec ts_2;
	int rc;

	ts_0.tv_sec = 1;
	ts_0.tv_nsec = 999999999UL;

	ts_1 = ts_0;

	ts_2.tv_sec = 2;
	ts_2.tv_nsec = 0;

	rc = time_timespec_cmp(&ts_0, &ts_1);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_cmp(&ts_1, &ts_0);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_cmp(&ts_0, &ts_2);
	CU_ASSERT_EQUAL(rc, -1);

	rc = time_timespec_cmp(&ts_2, &ts_0);
	CU_ASSERT_EQUAL(rc, 1);
}

static void test_timetools_diff(void)
{
	struct timespec ts_start;
	struct timespec ts_end;
	struct timespec ts_new_end;
	struct timespec ts_diff;
	uint64_t start;
	uint64_t end;
	uint64_t diff;
	int64_t delta;
	int rc;

	rc = time_get_monotonic(&ts_start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ns(&ts_start, &start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_get_monotonic(&ts_end);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ns(&ts_end, &end);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_diff(&ts_start, &ts_end, &ts_diff);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ns(&ts_diff, &diff);
	CU_ASSERT_EQUAL(rc, 0);

	delta = end - start;
	CU_ASSERT_EQUAL((int64_t)diff, delta);

	/* */
	rc = time_timespec_cmp(&ts_start, &ts_end);
	CU_ASSERT_EQUAL(rc, -1);

	rc = time_timespec_cmp(&ts_start, &ts_start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_cmp(&ts_end, &ts_start);
	CU_ASSERT_EQUAL(rc, 1);

	/* */
	rc = time_timespec_add_ns(&ts_start, delta, &ts_new_end);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_cmp(&ts_new_end, &ts_end);
	CU_ASSERT_EQUAL(rc, 0);
}

static void test_timetools_convert(void)
{
	struct timespec ts_0;
	struct timespec ts_1;
	struct timeval tv;
	uint64_t value;
	uint32_t ms;
	int rc;

	rc = time_get_monotonic(&ts_0);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ns(&ts_0, &value);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_ns_to_timespec(&value, &ts_1);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_cmp(&ts_0, &ts_1);
	CU_ASSERT_EQUAL(rc, 0);

	/* */
	tv.tv_sec = 1;
	tv.tv_usec = 1333;
	rc = time_timeval_to_ms(&tv, &ms);
	CU_ASSERT_EQUAL(rc, 0);
	CU_ASSERT_EQUAL(ms, 1001);

	rc = time_timeval_to_timespec(&tv, &ts_0);
	CU_ASSERT_EQUAL(rc, 0);
	CU_ASSERT_EQUAL(ts_0.tv_sec, 1);
	CU_ASSERT_EQUAL(ts_0.tv_nsec, 1333000);
}

static void test_timetools_add(void)
{
	struct timespec ts_0;
	struct timespec ts_1;
	struct timespec ts_2;
	int rc;

	/* */
	ts_0.tv_sec = 0;
	ts_0.tv_nsec = 999999999UL;
	rc = time_timespec_add_ns(&ts_0, 1, &ts_1);
	CU_ASSERT_EQUAL(rc, 0);

	CU_ASSERT_EQUAL(ts_1.tv_sec, 1);
	CU_ASSERT_EQUAL(ts_1.tv_nsec, 0);

	/* */
	rc = time_timespec_add_ns(&ts_1, -1, &ts_2);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_cmp(&ts_0, &ts_2);
	CU_ASSERT_EQUAL(rc, 0);
}

static void do_msleep(int delay_ms)
{
	int rc;
	struct timespec ts_start;
	struct timespec ts_end;
	uint64_t start;
	uint64_t end;
	int64_t delta_ms;

	rc = time_get_monotonic(&ts_start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ms(&ts_start, &start);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_msleep(delay_ms);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_get_monotonic(&ts_end);
	CU_ASSERT_EQUAL(rc, 0);

	rc = time_timespec_to_ms(&ts_end, &end);
	CU_ASSERT_EQUAL(rc, 0);

	delta_ms = end - start;
	CU_ASSERT(delta_ms >= delay_ms);
}

static void test_timetools_msleep(void)
{
	int delay_ms;
	int list_delay_ms[] = { 0, 59, 609, 1109 };
	unsigned int i;

	for (i = 0; i < SIZEOF_ARRAY(list_delay_ms); i++) {
		delay_ms = list_delay_ms[i];

		do_msleep(delay_ms);
	}
}

CU_TestInfo s_timetools_tests[] = {
	{(char *)"monotonic", &test_timetools_monotonic},
	{(char *)"cmp", &test_timetools_cmp},
	{(char *)"diff", &test_timetools_diff},
	{(char *)"add", &test_timetools_add},
	{(char *)"convert", &test_timetools_convert},
	{(char *)"msleep", &test_timetools_msleep},
	CU_TEST_INFO_NULL,
};
