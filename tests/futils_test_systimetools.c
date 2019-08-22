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

struct time_parse_data {
	const char *s;
	int ret;
	uint64_t epoch_sec;
	int32_t utc_off_sec;
};

struct time_format_data {
	const char *s;
	enum time_fmt fmt;
	uint64_t epoch_sec;
	int32_t utc_off_sec;
};

static void test_systimetools_parse(void)
{
	int ret;
	size_t i;
	uint64_t epoch_sec;
	int32_t utc_off_sec;

	static const struct time_parse_data time_data[] = {
		{ "1970-01-01T00:00:00+00:00", 0, 0, 0 },
		{ "1970-01-01T01:00:00+01:00", 0, 0, 3600 },
		{ "1969-12-31T23:00:00-01:00", 0, 0, -3600 },

		{ "2018-03-01T12:13:14+00:00", 0, 1519906394, 0 },
		{ "20180301T121314+0000", 0, 1519906394, 0 },
		{ "20180301T12:13:14+00:00", 0, 1519906394, 0 },
		{ "2018-03-01T121314+0000", 0, 1519906394, 0 },
		{ "20180301T121314+00:00", 0, 1519906394, 0 },
		{ "20180301T12:13:14+0000", 0, 1519906394, 0 },

		{ "2018-03-01T22:38:14+10:25", 0, 1519906394, 37500 },
		{ "20180301T223814+1025", 0, 1519906394, 37500 },
		{ "20180301T22:38:14+10:25", 0, 1519906394, 37500 },
		{ "2018-03-01T223814+1025", 0, 1519906394, 37500 },
		{ "20180301T223814+10:25", 0, 1519906394, 37500 },
		{ "20180301T22:38:14+1025", 0, 1519906394, 37500 },

		{ "2018-03-01T01:48:14-10:25", 0, 1519906394, -37500 },
		{ "20180301T014814-1025", 0, 1519906394, -37500 },
		{ "20180301T01:48:14-10:25", 0, 1519906394, -37500 },
		{ "2018-03-01T014814-1025", 0, 1519906394, -37500 },
		{ "20180301T014814-10:25", 0, 1519906394, -37500 },
		{ "20180301T01:48:14-1025", 0, 1519906394, -37500 },

		{ "2017-12-31T23:59:59+00:00", 0, 1514764799, 0 },
		{ "2018-01-01T00:59:59+01:00", 0, 1514764799, 3600 },
		{ "2017-12-31T22:59:59-01:00", 0, 1514764799, -3600 },
		{ "2018-01-01T01:29:59+01:30", 0, 1514764799, 5400 },

		{ "2018-03-02T14:12:13+00:00", 0, 1519999933, 0 },
		{ "2018-03-03T00:12:13+10:00", 0, 1519999933, 36000 },

		{ "1970-01-01T00:21:49Z", 0, 1309, 0 },

		{ "Mon, 13 Aug 2018 16:05:19 GMT", 0, 1534176319, 0 },
		{ "Mon, 06 Aug 2018 09:03:45 GMT", 0, 1533546225, 0 },
		{ "Mon, 6 Aug 2018 09:03:45 GMT", 0, 1533546225, 0 },
		{ "Mon, 13 Aug 2018 16:05:19 UT", 0, 1534176319, 0 },
		{ "Mon, 06 Aug 2018 09:03:45 UT", 0, 1533546225, 0 },
		{ "Mon, 6 Aug 2018 09:03:45 UT", 0, 1533546225, 0 },

		{ "2018-03-02t14:12:13+00:00", -EINVAL, 0, 0 },
		{ "2018-03-02T14:12:13+00;00", -EINVAL, 0, 0 },
		{ "2018_03_02T14:12:13+00:00", -EINVAL, 0, 0 },
		{ "2018-03-02T14;12;13+0000", -EINVAL, 0, 0 },
	};

	for (i = 0; i < sizeof(time_data) / sizeof(time_data[0]); i++) {
		const struct time_parse_data *data = &time_data[i];

		epoch_sec = 0;
		utc_off_sec = 0;
		ret = time_local_parse(data->s, &epoch_sec, &utc_off_sec);
		CU_ASSERT_EQUAL(ret, data->ret);
		CU_ASSERT_EQUAL(epoch_sec, data->epoch_sec);
		CU_ASSERT_EQUAL(utc_off_sec, data->utc_off_sec);

		if (ret != data->ret ||
				epoch_sec != data->epoch_sec ||
				utc_off_sec != data->utc_off_sec) {
			printf("\n%s %" PRIu64 "(%" PRIu64 ") %d(%d)\n",
				data->s,
				epoch_sec, data->epoch_sec,
				utc_off_sec, data->utc_off_sec);
		}
	}
}

static void test_systimetools_format(void)
{
	int ret;
	size_t i;
	char s[64] ="";

	static const struct time_format_data time_data[] = {
		{ "20180301T121314+0000", TIME_FMT_SHORT, 1519906394, 0 },
		{ "2018-03-01T12:13:14+00:00", TIME_FMT_LONG, 1519906394, 0 },

		{ "20180301T223814+1025", TIME_FMT_SHORT, 1519906394, 37500 },
		{ "2018-03-01T22:38:14+10:25", TIME_FMT_LONG,
				1519906394, 37500 },

		{ "20180301T014814-1025", TIME_FMT_SHORT, 1519906394, -37500 },
		{ "2018-03-01T01:48:14-10:25", TIME_FMT_LONG,
				1519906394, -37500 },

		{ "Mon, 13 Aug 2018 16:05:19 GMT", TIME_FMT_RFC1123,
				1534176319, 0 },
		{ "Mon, 6 Aug 2018 09:03:45 GMT", TIME_FMT_RFC1123,
				1533546225, 0 },
	};

	for (i = 0; i < sizeof(time_data) / sizeof(time_data[0]); i++) {
		const struct time_format_data *data = &time_data[i];

		memset(s, 0, sizeof(s));
		ret = time_local_format(data->epoch_sec, data->utc_off_sec,
				data->fmt, s, sizeof(s));
		CU_ASSERT_EQUAL(ret, 0);
		CU_ASSERT_STRING_EQUAL(s, data->s);

		if (ret != 0 || strcmp(s, data->s) != 0) {
			printf("\n%" PRIu64 " %d %s(%s)\n",
				data->epoch_sec, data->utc_off_sec,
				s, data->s);
		}
	}
}

CU_TestInfo s_systimetools_tests[] = {
	{(char *)"parse", &test_systimetools_parse},
	{(char *)"format", &test_systimetools_format},
	CU_TEST_INFO_NULL,
};
