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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "futils/systimetools.h"

#define ULOG_TAG systimetools
#include <ulog.h>
ULOG_DECLARE_TAG(systimetools);

#define TIME_DATE_FORMAT "%F"
#define TIME_HOUR_FORMAT "T%H%M%S%z"

#ifdef BUILD_LIBPUTILS
#include <putils/properties.h>
#define UTC_OFFSET_PROP_NAME "timezone.utc_offset.sec"
#endif

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

/* Return the number of seconds since 1970-01-01 (unix epoch)
 * of the date given by its year / month / day / hour / min / sec components.
 * A local timezome UTC offset (+/-) can also be given (0 if not used) */
static uint64_t time_ctx_mkepoch(const struct time_ctx *ctx)
{
	uint64_t time;
	const uint64_t nb_day_1970 = 719499;
	const uint64_t cumulative_nb_day_per_month[] = {
		0, 30, 61, 91, 122, 152, 183, 214, 244, 275, 305, 336, 367
	};

	if (!ctx)
		return 0;

	uint32_t year = ctx->tm.tm_year + 1900;
	uint32_t month = ctx->tm.tm_mon + 1; /* 1 -> 12 */
	uint32_t day = ctx->tm.tm_mday;      /* 1 -> 31 */
	uint32_t hour = ctx->tm.tm_hour;     /* 0 -> 23 */
	uint32_t min = ctx->tm.tm_min;       /* 0 -> 59 */
	uint32_t sec = ctx->tm.tm_sec;	     /* 0 -> 59 */
	int32_t utc_offset_sec = ctx->tm.tm_gmtoff;

	/* Shift the given date two months back in order to have february
	 * (and especially its leap day) at the end of the shifted year
	 * to simplify future calculations.
	 * january is now 11 / february is now 12 / march is now 1 / ... */
	month -= 2;
	if ((int)month <= 0) {
		month += 12;
		year -= 1;
	}

	/* Add the number of leap days between year 0 and the given year,
	 * the formula comes from the gregorian calendar leap years rule */
	time = year/4 - year/100 + year/400;

	/* Handle years / months / days */
	time += year * 365;
	time += cumulative_nb_day_per_month[month];
	time += day;

	/* Shift origin from 0 to 1970 */
	time -= nb_day_1970;

	/* Handle hours / minutes / seconds */
	time = (time * 24) + hour;
	time = (time * 60) + min;
	time = (time * 60) + sec;

	/* Shift time with the timezone offset from UTC */
	time -= utc_offset_sec;

	return time;
}

int time_ctx_get_local(struct time_ctx *ctx, uint64_t *epoch_sec,
		int32_t *utc_offset_sec)
{
	if (!ctx || !epoch_sec || !utc_offset_sec)
		return -EINVAL;

	if (ctx->fields != TIME_FIELD_ALL)
		return -EINPROGRESS;

	*epoch_sec = time_ctx_mkepoch(ctx);
	*utc_offset_sec = ctx->tm.tm_gmtoff;

	return 0;
}

int time_local_set(uint64_t epoch_sec, int32_t utc_offset_sec)
{
	struct timeval tv;
	int ret;

	memset(&tv, 0, sizeof(tv));

	tv.tv_sec = epoch_sec;

#ifdef BUILD_LIBPUTILS
	/* utc offset */
	char value[SYS_PROP_VALUE_MAX];
	snprintf(value, sizeof(value), "%d", utc_offset_sec);
	ret = sys_prop_set(UTC_OFFSET_PROP_NAME, value);
	if (ret < 0)
		return ret;
#endif

	ret = settimeofday(&tv, NULL);
	if (ret < 0)
		return -errno;

	return 0;
}

int time_local_get(uint64_t *epoch_sec, int32_t *utc_offset_sec)
{
	struct timeval tv;
	int ret;

	if (!epoch_sec)
		return -EINVAL;

	ret = gettimeofday(&tv, NULL);
	if (ret < 0)
		return -errno;

	*epoch_sec = tv.tv_sec;

	if (!utc_offset_sec)
		return 0;

	*utc_offset_sec = 0;

#ifdef BUILD_LIBPUTILS
	/* get the current timezone utc offset from property */
	char prop[SYS_PROP_VALUE_MAX];
	sys_prop_get(UTC_OFFSET_PROP_NAME, prop, "0");
	*utc_offset_sec = strtol(prop, NULL, 10);
#endif

	return 0;
}

int time_local_create_tm(uint64_t epoch_sec, int32_t utc_offset_sec,
		struct tm *tm)
{
	if (!tm)
		return -EINVAL;

	if (gmtime_r((time_t *)&epoch_sec, tm) == NULL)
		return -errno;

	tm->tm_gmtoff = utc_offset_sec;

	return 0;
}

int time_local_format(uint64_t epoch_sec, int32_t utc_offset_sec,
		char *date, size_t datesize, char *hour, size_t hoursize)
{
	int res;
	size_t ret;
	struct tm tm;

	/* convert epoch_sec to local time */
	epoch_sec += utc_offset_sec;

	res = time_local_create_tm(epoch_sec, utc_offset_sec, &tm);
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
