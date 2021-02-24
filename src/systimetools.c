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

/* For gmtime_r access */
#ifdef _WIN32
#  define _POSIX_C_SOURCE 2
#  include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "futils/systimetools.h"

#define ULOG_TAG systimetools
#include <ulog.h>
ULOG_DECLARE_TAG(systimetools);

#ifdef BUILD_LIBPUTILS
#include <putils/properties.h>
#define UTC_OFFSET_PROP_NAME "timezone.utc_offset.sec"
#endif

#define TIME_FIELD_DATE (1 << 0)
#define TIME_FIELD_HOUR (1 << 1)
#define TIME_FIELD_ALL (TIME_FIELD_DATE|TIME_FIELD_HOUR)


static const char *wday_str[7] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
};


static const char *mon_str[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
};


static int parse_num(const char *s, int *num)
{
	char *end = NULL;
	errno = 0;
	*num = strtol(s, &end, 10);
	return (end == NULL || *end != '\0') ? -EINVAL : -errno;
}

static int parse_wday(const char *s, int *num)
{
	int i, ret = -ENOENT;
	for (i = 0; i < 7; i++) {
		if (strncmp(s, wday_str[i], 3) == 0) {
			if (num)
				*num = i;
			ret = 0;
			break;
		}
	}
	return ret;
}

static int parse_mon(const char *s, int *num)
{
	int i, ret = -ENOENT;
	for (i = 0; i < 12; i++) {
		if (strncmp(s, mon_str[i], 3) == 0) {
			if (num)
				*num = i + 1;
			ret = 0;
			break;
		}
	}
	return ret;
}

/*
 * YYYY-MM-DD
 * YYYYMMDD
 */
static int parse_date(const char *s, size_t n, int *year, int *mon, int *mday)
{
	int ret;
	char year_s[5], mon_s[3], mday_s[3];

	if (n == 10 && s[4] == '-' && s[7] == '-') {
		year_s[0] = s[0];
		year_s[1] = s[1];
		year_s[2] = s[2];
		year_s[3] = s[3];
		year_s[4] = '\0';
		mon_s[0] = s[5];
		mon_s[1] = s[6];
		mon_s[2] = '\0';
		mday_s[0] = s[8];
		mday_s[1] = s[9];
		mday_s[2] = '\0';
	} else if (n == 8) {
		year_s[0] = s[0];
		year_s[1] = s[1];
		year_s[2] = s[2];
		year_s[3] = s[3];
		year_s[4] = '\0';
		mon_s[0] = s[4];
		mon_s[1] = s[5];
		mon_s[2] = '\0';
		mday_s[0] = s[6];
		mday_s[1] = s[7];
		mday_s[2] = '\0';
	} else {
		return -EINVAL;
	}

	ret = parse_num(year_s, year);
	if (ret < 0)
		return ret;
	ret = parse_num(mon_s, mon);
	if (ret < 0)
		return ret;
	ret = parse_num(mday_s, mday);
	if (ret < 0)
		return ret;

	return ret;
}

/*
 * aaa, DD bbb YYYY
 */
static int parse_date_rfc1123(const char *s, size_t n,
		int *year, int *mon, int *mday)
{
	int ret, offset = 0;
	char year_s[5], mon_s[4], mday_s[3], wday_s[4];

	if (s[3] != ',')
		return -EINVAL;

	wday_s[0] = s[offset++];
	wday_s[1] = s[offset++];
	wday_s[2] = s[offset++];
	wday_s[3] = '\0';
	offset += 2;
	mday_s[0] = s[offset++];
	if (s[offset] == ' ') {
		/* 1 digit month day (eg. '5') */
		mday_s[1] = '\0';
		mday_s[2] = '\0';
	} else {
		/* 2 digit month day, including leading 0 (eg. '05' or '23') */
		mday_s[1] = s[offset++];
		mday_s[2] = '\0';
	}
	offset++;
	mon_s[0] = s[offset++];
	mon_s[1] = s[offset++];
	mon_s[2] = s[offset++];
	mon_s[3] = '\0';
	offset++;
	year_s[0] = s[offset++];
	year_s[1] = s[offset++];
	year_s[2] = s[offset++];
	year_s[3] = s[offset++];
	year_s[4] = '\0';

	ret = parse_num(year_s, year);
	if (ret < 0)
		return ret;
	ret = parse_mon(mon_s, mon);
	if (ret < 0)
		return ret;
	ret = parse_num(mday_s, mday);
	if (ret < 0)
		return ret;

	return offset;
}

/*
 * Leading 'T' optional
 * Mix short/long for hour/timezone allowed
 *
 * hh:mm:ss
 * hhmmss
 *
 * zzz:zz +01:00
 * zzzzz +0100
 */
static int parse_time(const char *s, size_t n,
		int *hour, int *min, int *sec, int *gmtoff)
{
	int ret;
	char hour_s[3], min_s[3], sec_s[3];
	int gmtoff_sign, gmtoff_hour, gmtoff_min;
	char gmtoff_hour_s[3], gmtoff_min_s[3];

	/* Skip 'T' if present */
	if (n > 0 && s[0] == 'T') {
		s++;
		n--;
	}

	/* Determine time format */
	if (n >= 8 && s[2] == ':' && s[5] == ':') {
		hour_s[0] = s[0];
		hour_s[1] = s[1];
		hour_s[2] = '\0';
		min_s[0] = s[3];
		min_s[1] = s[4];
		min_s[2] = '\0';
		sec_s[0] = s[6];
		sec_s[1] = s[7];
		sec_s[2] = '\0';
		s += 8;
		n -= 8;
	} else if (n >= 6) {
		hour_s[0] = s[0];
		hour_s[1] = s[1];
		hour_s[2] = '\0';
		min_s[0] = s[2];
		min_s[1] = s[3];
		min_s[2] = '\0';
		sec_s[0] = s[4];
		sec_s[1] = s[5];
		sec_s[2] = '\0';
		s += 6;
		n -= 6;
	} else {
		return -EINVAL;
	}

	/* Parse time fields */
	ret = parse_num(hour_s, hour);
	if (ret < 0)
		return ret;
	ret = parse_num(min_s, min);
	if (ret < 0)
		return ret;
	ret = parse_num(sec_s, sec);
	if (ret < 0)
		return ret;

	/* Skip space if needed */
	if (n > 0 && s[0] == ' ') {
		s++;
		n--;
	}

	/* Determine time zome format (independently from time in case there
	 * is a mix of short/long */
	if (n == 6 && (s[0] == '-' || s[0] == '+') && s[3] == ':') {
		gmtoff_sign = s[0] == '-' ? -1 : 1;
		gmtoff_hour_s[0] = s[1];
		gmtoff_hour_s[1] = s[2];
		gmtoff_hour_s[2] = '\0';
		gmtoff_min_s[0] = s[4];
		gmtoff_min_s[1] = s[5];
		gmtoff_min_s[2] = '\0';
	} else if (n == 5 && (s[0] == '-' || s[0] == '+')) {
		gmtoff_sign = s[0] == '-' ? -1 : 1;
		gmtoff_hour_s[0] = s[1];
		gmtoff_hour_s[1] = s[2];
		gmtoff_hour_s[2] = '\0';
		gmtoff_min_s[0] = s[3];
		gmtoff_min_s[1] = s[4];
		gmtoff_min_s[2] = '\0';
	} else if (n == 1 && (s[0] == 'Z')) {
		gmtoff_sign = 1;
		gmtoff_hour_s[0] = '0';
		gmtoff_hour_s[1] = '0';
		gmtoff_hour_s[2] = '\0';
		gmtoff_min_s[0] = '0';
		gmtoff_min_s[1] = '0';
		gmtoff_min_s[2] = '\0';
	} else if (n == 3 && (s[0] == 'G') && (s[1] == 'M') && (s[2] == 'T')) {
		gmtoff_sign = 1;
		gmtoff_hour_s[0] = '0';
		gmtoff_hour_s[1] = '0';
		gmtoff_hour_s[2] = '\0';
		gmtoff_min_s[0] = '0';
		gmtoff_min_s[1] = '0';
		gmtoff_min_s[2] = '\0';
	} else if (n == 2 && (s[0] == 'U') && (s[1] == 'T')) {
		gmtoff_sign = 1;
		gmtoff_hour_s[0] = '0';
		gmtoff_hour_s[1] = '0';
		gmtoff_hour_s[2] = '\0';
		gmtoff_min_s[0] = '0';
		gmtoff_min_s[1] = '0';
		gmtoff_min_s[2] = '\0';
	} else {
		return -EINVAL;
	}

	ret = parse_num(gmtoff_hour_s, &gmtoff_hour);
	if (ret < 0)
		return ret;
	ret = parse_num(gmtoff_min_s, &gmtoff_min);
	if (ret < 0)
		return ret;
	*gmtoff = (gmtoff_hour * 60 + gmtoff_min) * 60 * gmtoff_sign;

	return 0;
}

static int parse_date_time(const char *s, size_t n,
		int *year, int *mon, int *mday,
		int *hour, int *min, int *sec, int *gmtoff)
{
	int ret;

	if ((n >= 16) && (parse_wday(s, NULL) != -ENOENT)) {
		/* RFC 1123 date */
		ret = parse_date_rfc1123(s, 16, year, mon, mday);
		if (ret < 0)
			return ret;
		s += ret;
		n -= ret;
	} else if (n >= 10 && (s[10] == 'T' || s[10] == ' ' || s[10] == '\0')) {
		/* ISO 8601 long date */
		ret = parse_date(s, 10, year, mon, mday);
		if (ret < 0)
			return ret;
		s += 10;
		n -= 10;
	} else if (n >= 8 && (s[8] == 'T' || s[8] == ' ' || s[8] == '\0')) {
		/* ISO 8601 short date */
		ret = parse_date(s, 8, year, mon, mday);
		if (ret < 0)
			return ret;
		s += 8;
		n -= 8;
	} else {
		return -EINVAL;
	}

	/* Skip space if needed (keep 'T') */
	if (n > 0 && s[0] == ' ') {
		s++;
		n--;
	}

	ret = parse_time(s, n, hour, min, sec, gmtoff);
	if (ret < 0)
		return ret;

	return 0;
}


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

	if (!ctx || !str_date)
		return -EINVAL;

	/* Avoid to set the field twice */
	if (ctx->fields & TIME_FIELD_DATE)
		return -EEXIST;

	/* Parse date */
	ret = parse_date(str_date, strlen(str_date),
			&ctx->tm.tm_year, &ctx->tm.tm_mon, &ctx->tm.tm_mday);
	if (ret < 0)
		return ret;
	ctx->tm.tm_year -= 1900;
	ctx->tm.tm_mon -= 1;

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

	if (!ctx || !str_hour)
		return -EINVAL;

	if (ctx->fields & TIME_FIELD_HOUR)
		return -EEXIST;

	ret = parse_time(str_hour, strlen(str_hour),
			&ctx->tm.tm_hour, &ctx->tm.tm_min, &ctx->tm.tm_sec,
			&ctx->gmtoff);
	if (ret < 0)
		return ret;

	ctx->fields |= TIME_FIELD_HOUR;

	/* Check if complete time is known */
	if (ctx->fields == TIME_FIELD_ALL)
		ret = 0;
	else
		ret = -EINPROGRESS;

	return ret;
}

int time_ctx_set_time(struct time_ctx *ctx, const char *str_time)
{
	int ret;

	if (!ctx || !str_time)
		return -EINVAL;

	/* Avoid to set the field twice */
	if (ctx->fields & TIME_FIELD_ALL)
		return -EEXIST;

	/* Parse date and time */
	ret = parse_date_time(str_time, strlen(str_time),
			&ctx->tm.tm_year, &ctx->tm.tm_mon, &ctx->tm.tm_mday,
			&ctx->tm.tm_hour, &ctx->tm.tm_min, &ctx->tm.tm_sec,
			&ctx->gmtoff);
	if (ret < 0)
		return ret;
	ctx->tm.tm_year -= 1900;
	ctx->tm.tm_mon -= 1;

	ctx->fields = TIME_FIELD_ALL;
	return 0;
}

/* Return the number of seconds since 1970-01-01 (unix epoch)
 * of the date given by its year / month / day / hour / min / sec components.
 * A local timezome UTC offset (+/-) can also be given (0 if not used) */
static uint64_t tm_mkepoch_local(const struct tm *tm, int gmtoff)
{
	uint64_t time;
	const uint64_t nb_day_1970 = 719499;
	const uint64_t cumulative_nb_day_per_month[] = {
		0, 30, 61, 91, 122, 152, 183, 214, 244, 275, 305, 336, 367
	};

	if (!tm)
		return 0;

	uint32_t year = tm->tm_year + 1900;
	uint32_t month = tm->tm_mon + 1; /* 1 -> 12 */
	uint32_t day = tm->tm_mday;      /* 1 -> 31 */
	uint32_t hour = tm->tm_hour;     /* 0 -> 23 */
	uint32_t min = tm->tm_min;       /* 0 -> 59 */
	uint32_t sec = tm->tm_sec;	     /* 0 -> 59 */
	int32_t utc_offset_sec = gmtoff;

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

static uint64_t time_ctx_mkepoch_local(const struct time_ctx *ctx)
{
	if (!ctx)
		return 0;

	return tm_mkepoch_local(&ctx->tm, ctx->gmtoff);
}

int time_ctx_get_local(struct time_ctx *ctx, uint64_t *epoch_sec,
		int32_t *utc_offset_sec)
{
	if (!ctx || !epoch_sec || !utc_offset_sec)
		return -EINVAL;

	if (ctx->fields != TIME_FIELD_ALL)
		return -EINPROGRESS;

	*epoch_sec = time_ctx_mkepoch_local(ctx);
	*utc_offset_sec = ctx->gmtoff;

	return 0;
}

int time_local_set(uint64_t epoch_sec, int32_t utc_offset_sec)
{
#ifdef _WIN32
	return -ENOSYS;
#else
	struct timeval tv;
	int ret;

	memset(&tv, 0, sizeof(tv));

	tv.tv_sec = epoch_sec;

	ret = settimeofday(&tv, NULL);
	if (ret < 0)
		return -errno;

#ifdef BUILD_LIBPUTILS
	/* utc offset */
	char value[SYS_PROP_VALUE_MAX];
	snprintf(value, sizeof(value), "%d", utc_offset_sec);
	ret = sys_prop_set(UTC_OFFSET_PROP_NAME, value);
	if (ret < 0)
		return ret;
#endif

	return 0;
#endif
}

int time_local_get(uint64_t *epoch_sec, int32_t *utc_offset_sec)
{
	return time_local_ms_get(epoch_sec, NULL, utc_offset_sec);
}

int time_local_ms_get(uint64_t *epoch_sec, uint16_t *ms,
		int32_t *utc_offset_sec)
{
	struct timeval tv;
	int ret;

	if (!epoch_sec && !utc_offset_sec)
		return -EINVAL;

	ret = gettimeofday(&tv, NULL);
	if (ret < 0)
		return -errno;

	if (epoch_sec)
		*epoch_sec = tv.tv_sec;

	if (ms)
		*ms = tv.tv_usec / 1000;

	if (!utc_offset_sec)
		return 0;

	*utc_offset_sec = 0;

#ifdef BUILD_LIBPUTILS
	/* get the current timezone utc offset from property */
	char prop[SYS_PROP_VALUE_MAX];
	sys_prop_get(UTC_OFFSET_PROP_NAME, prop, "0");
	*utc_offset_sec = strtol(prop, NULL, 10);
#elif defined(_WIN32)
	TIME_ZONE_INFORMATION tzi;
	memset(&tzi, 0, sizeof(tzi));
	GetTimeZoneInformation(&tzi);
	*utc_offset_sec = tzi.Bias * 60;
#else
	time_t dt;
	struct tm tm;
	time(&dt);
	localtime_r(&dt, &tm);
	*utc_offset_sec = tm.tm_gmtoff;
#endif

	return 0;
}

int time_local_to_tm(uint64_t epoch_sec, int32_t utc_offset_sec,
		struct tm *tm)
{
	time_t t;
	if (!tm)
		return -EINVAL;

	/* convert epoch_sec to local time */
	t = epoch_sec + utc_offset_sec;
	if (gmtime_r(&t, tm) == NULL)
		return -errno;

#ifndef _WIN32
	tm->tm_gmtoff = utc_offset_sec;
#endif /* !_WIN32 */

	return 0;
}

int time_local_from_tm(const struct tm *tm, uint64_t *epoch_sec,
		int32_t *utc_offset_sec)
{
	if (!tm)
		return -EINVAL;

	if (!epoch_sec && !utc_offset_sec)
		return -EINVAL;

#ifdef _WIN32
	if (epoch_sec)
		*epoch_sec = tm_mkepoch_local(tm, 0);

	if (utc_offset_sec)
		*utc_offset_sec = 0;
#else /* !_WIN32 */
	if (epoch_sec)
		*epoch_sec = tm_mkepoch_local(tm, tm->tm_gmtoff);

	if (utc_offset_sec)
		*utc_offset_sec = tm->tm_gmtoff;
#endif /* !_WIN32 */
	return 0;
}

int time_local_format(uint64_t epoch_sec, int32_t utc_offset_sec,
		enum time_fmt fmt, char *s, size_t n)
{
	time_t t;
	struct tm tm;
	int gmtoff_sign, gmtoff_hour, gmtoff_min;
	if (!s || !n)
		return -EINVAL;

	/* convert epoch_sec to local time */
	t = epoch_sec + utc_offset_sec;
	if (gmtime_r(&t, &tm) == NULL)
		return -errno;

	/* Time zone offset */
	gmtoff_sign = (utc_offset_sec < 0) ? -1 : 1;
	gmtoff_hour = gmtoff_sign * utc_offset_sec / (60 * 60);
	gmtoff_min = (gmtoff_sign * utc_offset_sec / 60) % 60;

	switch (fmt) {
	case TIME_FMT_ISO8601_SHORT:
		snprintf(s, n, "%04u%02u%02uT%02u%02u%02u%c%02u%02u",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			gmtoff_sign < 0 ? '-' : '+',
			gmtoff_hour, gmtoff_min);
		break;
	case TIME_FMT_ISO8601_LONG:
		snprintf(s, n, "%04u-%02u-%02uT%02u:%02u:%02u%c%02u:%02u",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			gmtoff_sign < 0 ? '-' : '+',
			gmtoff_hour, gmtoff_min);
		break;
	case TIME_FMT_RFC1123:
		if (utc_offset_sec == 0) {
			/* TODO: "GMT" should not be used any more, use "UT"
			 * instead; "GMT" is kept for now for compatibility
			 * issues with old software that will reject "UT"
			 * in time_local_parse(); update to "UT" here once
			 * we consider old software is no longer in use. */
			snprintf(s, n, "%s, %u %s %u %02u:%02u:%02u GMT",
				wday_str[tm.tm_wday], tm.tm_mday,
				mon_str[tm.tm_mon], tm.tm_year + 1900,
				tm.tm_hour, tm.tm_min, tm.tm_sec);
		} else {
			snprintf(s, n, "%s, %u %s %u %02u:%02u:%02u %c%02u%02u",
				wday_str[tm.tm_wday], tm.tm_mday,
				mon_str[tm.tm_mon], tm.tm_year + 1900,
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				gmtoff_sign < 0 ? '-' : '+',
				gmtoff_hour, gmtoff_min);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int time_local_parse(const char *s, uint64_t *epoch_sec,
		int32_t *utc_offset_sec)
{
	int ret;
	struct tm tm;
	int gmtoff;

	if (!s || !epoch_sec || !utc_offset_sec)
		return -EINVAL;

	ret = parse_date_time(s, strlen(s),
			&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
			&gmtoff);
	if (ret < 0)
		return ret;
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;

	*epoch_sec = tm_mkepoch_local(&tm, gmtoff);
	*utc_offset_sec = gmtoff;
	return 0;
}
