/******************************************************************************
 * Copyright (c) 2016 Parrot S.A.
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
 * @file systimetools.c
 *
 * @brief drone system time tools.
 *
 ******************************************************************************/

#include <string.h>
#include <errno.h>
#include "futils/systimetools.h"

#define ULOG_TAG systimetools
#include <ulog.h>
ULOG_DECLARE_TAG(systimetools);

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
		int32_t *minuteswest)
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

	/**
	 * see ftp://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_chapter/libc_21.html#SEC441
	 * for timezone
	 * tm_gmtoff unit is Seconds east of UTC
	 */
	*minuteswest = -ctx->tm.tm_gmtoff / 60;

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

int time_system_create_tm(uint64_t unix_usec, int32_t minuteswest,
		struct tm *tm)
{
	time_t unix_sec;

	if (!tm)
		return -EINVAL;

	unix_sec = unix_usec / US_TO_SEC;
	if (gmtime_r(&unix_sec, tm) == NULL)
		return -errno;

	/* tm_gmtoff unit is seconds */
	tm->tm_gmtoff = -minuteswest * 60;

	return 0;
}

int time_system_convert_time(uint64_t unix_usec, int32_t minuteswest,
		char *date, size_t datesize, char *hour, size_t hoursize)
{
	int res;
	size_t ret;
	struct tm tm;

	res = time_system_create_tm(unix_usec, minuteswest, &tm);
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
