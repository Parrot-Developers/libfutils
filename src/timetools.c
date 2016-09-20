/******************************************************************************
 * Copyright (c) 2015 Parrot S.A.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file timetools.c
 *
 * @brief drone time tools.
 *
 ******************************************************************************/

#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "futils/timetools.h"

#define ULOG_TAG timetools
#include <ulog.h>
ULOG_DECLARE_TAG(timetools);

#define TIME_DATE_FORMAT "%F"
#define TIME_HOUR_FORMAT "T%H%M%S%z"

#define TIME_FIELD_DATE (1 << 0)
#define TIME_FIELD_HOUR (1 << 1)
#define TIME_FIELD_ALL (TIME_FIELD_DATE|TIME_FIELD_HOUR)

#define US_TO_SEC 1000000

int time_ctx_init(struct time_ctx *ctx)
{
	if (!ctx)
		return -EINVAL;

	memset(ctx, 0, sizeof(struct time_ctx));

	return 0;
}

int time_ctx_set_date(struct time_ctx *ctx, const char *str_date)
{
	int ret;
	char *enddate;
	struct tm tm;

	if (!ctx || !str_date)
		return -EINVAL;

	/* Avoid to set the field twice */
	if (ctx->fields & TIME_FIELD_DATE)
		return -EEXIST;

	/* Parse date */
	memset(&tm, 0, sizeof(tm));
	enddate = strptime(str_date, TIME_DATE_FORMAT, &tm);

	/* Reject invalid dates */
	if (enddate == NULL || *enddate != '\0')
		return -EINVAL;

	/* Copy fields related to date */
	ctx->tm.tm_mday = tm.tm_mday;
	ctx->tm.tm_mon = tm.tm_mon;
	ctx->tm.tm_year = tm.tm_year;
	ctx->tm.tm_wday = tm.tm_wday;
	ctx->tm.tm_yday = tm.tm_yday;

	ctx->fields |= TIME_FIELD_DATE;

	/* Check if complete time is known */
	if (ctx->fields == TIME_FIELD_ALL)
		ret = 0;
	else
		ret = -EINPROGRESS;

	return ret;
}

int time_ctx_set_hour(struct time_ctx *ctx, const char *str_hour)
{
	int ret;
	char *endhour;
	struct tm tm;

	if (!ctx || !str_hour)
		return -EINVAL;

	if (ctx->fields & TIME_FIELD_HOUR)
		return -EEXIST;

	memset(&tm, 0, sizeof(tm));
	endhour = strptime(str_hour, TIME_HOUR_FORMAT, &tm);

	/* Reject invalid dates */
	if (endhour == NULL || *endhour != '\0')
		return -EINVAL;

	/* Copy fields related to hour */
	ctx->tm.tm_sec = tm.tm_sec;
	ctx->tm.tm_min = tm.tm_min;
	ctx->tm.tm_hour = tm.tm_hour;
	ctx->tm.tm_gmtoff = tm.tm_gmtoff;

	ctx->fields |= TIME_FIELD_HOUR;

	/* Check if complete time is known */
	if (ctx->fields == TIME_FIELD_ALL)
		ret = 0;
	else
		ret = -EINPROGRESS;

	return ret;
}

int time_ctx_get_time(struct time_ctx *ctx, uint64_t *time_unix_usec,
		uint32_t *minuteswest)
{
	struct tm tm;

	if (!ctx || !time_unix_usec || !minuteswest)
		return -EINVAL;

	if (ctx->fields != TIME_FIELD_ALL)
		return -EINPROGRESS;

	/*
	 * mktime() change the value of its parameter, we need to work on a
	 * copy to avoid problems.
	 */
	memcpy(&tm, &ctx->tm, sizeof(tm));
	*time_unix_usec = (uint64_t) mktime(&tm) * US_TO_SEC;

	*minuteswest = ctx->tm.tm_gmtoff / 60; /* tm_gmtoff unit is seconds */

	return 0;
}

int time_system_set_time(uint64_t unix_usec, int32_t minuteswest)
{
	struct timeval tv;
	struct timezone tz;
	int ret;

	memset(&tv, 0, sizeof(tv));
	memset(&tz, 0, sizeof(tz));

	tv.tv_sec = unix_usec / US_TO_SEC;
	tv.tv_usec = unix_usec % US_TO_SEC;

	tz.tz_minuteswest = minuteswest;

	ret = settimeofday(&tv, &tz);
	if (ret < 0)
		return -errno;

	return 0;
}

int time_system_get_time(uint64_t *unix_usec, int32_t *minuteswest)
{
	struct timeval tv;
	struct timezone tz;
	int ret;

	if (!unix_usec || !minuteswest)
		return -EINVAL;

	ret = gettimeofday(&tv, &tz);
	if (ret < 0)
		return -errno;

	*unix_usec = tv.tv_usec;
	*unix_usec += (uint64_t) tv.tv_sec * US_TO_SEC;
	*minuteswest = tz.tz_minuteswest;
	return 0;
}

static int time_create_tm(uint64_t unix_usec, int32_t minuteswest,
		struct tm *tm)
{
	time_t unix_sec;

	unix_sec = unix_usec / US_TO_SEC;
	if (gmtime_r(&unix_sec, tm) == NULL)
		return -errno;

	/* tm_gmtoff unit is seconds */
	tm->tm_gmtoff = minuteswest * 60;

	return 0;
}

int time_system_convert_time(uint64_t unix_usec, int32_t minuteswest,
		char *date, size_t datesize, char *hour, size_t hoursize)
{
	int res;
	size_t ret;
	struct tm tm;

	res = time_create_tm(unix_usec, minuteswest, &tm);
	if (res < 0)
		return res;

	ret = strftime(date, datesize, TIME_DATE_FORMAT, &tm);
	if (ret == 0)
		return -errno;

	ret = strftime(hour, hoursize, TIME_HOUR_FORMAT, &tm);
	if (ret == 0)
		return -errno;

	return 0;
}

int time_timespec_diff(const struct timespec *start,
		       const struct timespec *end,
		       struct timespec *diff)

{
	if (!start || !end || !diff)
		return -EINVAL;

	if ((end->tv_sec < start->tv_sec) ||
	   ((end->tv_sec == start->tv_sec) && (end->tv_nsec < start->tv_nsec)))
		return -EINVAL;

	/* diff = end - start */
	if (end->tv_nsec > start->tv_nsec) {
		diff->tv_sec = end->tv_sec - start->tv_sec;
		diff->tv_nsec = end->tv_nsec - start->tv_nsec;
	} else {
		diff->tv_sec = end->tv_sec - start->tv_sec - 1;
		diff->tv_nsec = 1000000000UL + end->tv_nsec - start->tv_nsec;
	}

	return 0;
}

int time_timespec_diff_in_range(const struct timespec *t1,
				const struct timespec *t2,
				uint64_t range_us,
				uint64_t *out_diff_us)
{
	struct timespec diff;
	uint64_t diff_us = 0;
	int ret;

	if (!t1 || !t2)
		return 0;

	memset(&diff, 0, sizeof(diff));

	ret = time_timespec_cmp(t1, t2);
	if (ret == 0)
		return 1;

	if (ret == 1) {
		ret = time_timespec_diff(t2, t1, &diff);
		if (ret < 0)
			return 0;
	} else {
		ret = time_timespec_diff(t1, t2, &diff);
		if (ret < 0)
			return 0;
	}

	time_timespec_to_us(&diff, &diff_us);
	if (out_diff_us)
		*out_diff_us = diff_us;

	return diff_us < range_us;
}

int time_timespec_to_us(const struct timespec *value, uint64_t *us)
{
	if (!value || !us)
		return -EINVAL;

	*us = (uint64_t)value->tv_sec * 1000000UL + (value->tv_nsec / 1000UL);
	return 0;
}

int time_ns_to_timespec(const uint64_t *value, struct timespec *ts)
{
	if (!value || !ts)
		return -EINVAL;

	ts->tv_sec = *value / 1000000000UL;
	ts->tv_nsec = (*value % 1000000000UL);
	return 0;
}

int time_us_to_timespec(const uint64_t *value, struct timespec *ts)
{
	if (!value || !ts)
		return -EINVAL;

	ts->tv_sec = *value / 1000000UL;
	ts->tv_nsec = (*value % 1000000UL) * 1000UL;
	return 0;
}

int time_timespec_diff_now(const struct timespec *value, struct timespec *diff)
{
	int ret;
	struct timespec now;

	if (!diff || !value)
		return -EINVAL;

	ret = clock_gettime(CLOCK_MONOTONIC, &now);
	if (ret < 0) {
		ULOGE("clock_gettime error: %m");
		return -errno;
	}

	return time_timespec_diff(value, &now, diff);

}

int time_timespec_cmp(const struct timespec *t1, const struct timespec *t2)
{
	int ret = 0;

	if (!t1 || !t2) {
		ULOGE("Null argument in time_timespec_cmp");
		goto exit;
	}

	if (t1->tv_sec > t2->tv_sec) {
		ret = 1;
	} else if (t1->tv_sec < t2->tv_sec) {
		ret = -1;
	} else {
		if (t1->tv_nsec > t2->tv_nsec)
			ret = 1;
		else if (t1->tv_nsec < t2->tv_nsec)
			ret = -1;
		else
			ret = 0;
	}

exit:
	return ret;
}

int time_timespec_add_ns(const struct timespec *ts, int64_t delta,
			 struct timespec *res)
{
	int64_t sec = 0, ns = 0;
	if (!ts || !res)
		return -EINVAL;

	sec = delta / 1000000000;
	ns =  delta % 1000000000;

	res->tv_sec = ts->tv_sec + sec;
	res->tv_nsec = ts->tv_nsec + ns;
	if (res->tv_nsec >= 1000000000) {
		res->tv_nsec -= 1000000000;
		res->tv_sec++;
	} else if (res->tv_nsec < 0) {
		res->tv_nsec += 1000000000;
		res->tv_sec--;
	}

	return 0;
}

int time_timespec_add_us(const struct timespec *ts, int64_t delta,
			 struct timespec *res)
{
	return time_timespec_add_ns(ts, delta * 1000, res);
}

int time_timeval_to_timespec(const struct timeval *tv, struct timespec *ts)
{
	if (!tv || !ts)
		return -EINVAL;

	ts->tv_sec = tv->tv_sec;
	ts->tv_nsec = tv->tv_usec * 1000;

	return 0;
}

int time_timeval_to_ms(const struct timeval *value, uint32_t *ms)
{
	if (!value || !ms)
		return -EINVAL;

	*ms = (uint32_t)value->tv_sec * 1000UL + (value->tv_usec / 1000UL);
	return 0;
}
