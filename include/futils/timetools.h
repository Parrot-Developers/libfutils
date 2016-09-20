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
 * @file timetools.h
 *
 * @brief drone time tools.
 *
 ******************************************************************************/

#ifndef _TIMETOOLS_H_
#define _TIMETOOLS_H_

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Context to track time configuration.
 *
 * Current public messages send the system time with two successive messages :
 * one to set the date (YYYY-MM-DD), one other to set the hour (HH-MM-SS).
 *
 * This context allows the configuration of the system time with these two
 * messages, regardless of the reception order.
 */
struct time_ctx {
	int fields;
	struct tm tm; /* tm.tm_isdst is ignored */
};

/**
 * @brief Initialize the time context
 *
 * @return 0 Initialization ok
 * @return -EINVAL Invalid parameter
 */
int time_ctx_init(struct time_ctx *ctx);

/**
 * @brief Set the date of the system
 *
 * @return 0 Both date and hour have been configured. The system has been
 *           configured.
 * @return -EINPROGRESS Date as been saved but hour has not been configured yet
 * @return -EINVAL Invalid parameter
 * @return -EEXIST The date has already been set
 */
int time_ctx_set_date(struct time_ctx *ctx, const char *str_date);

/**
 * @brief Set the hour of the system
 *
 * @return 0 Both date and hour have been configured. The system has been
 *           configured.
 * @return -EINPROGRESS Hour as been saved but date has not been configured yet
 * @return -EINVAL Invalid parameter
 * @return -EEXIST The hour has already been set
 */
int time_ctx_set_hour(struct time_ctx *ctx, const char *str_hour);

/**
 * @brief Get the stored time
 *
 * @return 0 If ctx contains a valid time
 * @return -EINPROGRESS If ctx is not completed
 */
int time_ctx_get_time(struct time_ctx *ctx, uint64_t *time_unix_usec,
		uint32_t *minuteswest);

/**
 * System time API
 */

/**
 * @brief Set the time of the system
 *
 * @param unix_usec Timestamp of the master clock in microseconds since UNIX
 *                  epoch.
 * @param minuteswest Timestamp of the component clock since boot time in
 *                    milliseconds.
 * @return 0 on success
 */
int time_system_set_time(uint64_t unix_usec, int32_t minuteswest);

/**
 * @brief Get the time of the system
 *
 * @param unix_usec Timestamp of the master clock in microseconds since UNIX
 *                  epoch.
 * @param minuteswest Timestamp of the component clock since boot time in
 *                    milliseconds.
 * @return 0 on success
 * @return -EINVAL if at last one parameter is NULL
 */
int time_system_get_time(uint64_t *unix_usec, int32_t *minuteswest);

/**
 * @brief Get a formated representation of the system time.
 *
 * @param date The string that will contain the formated date
 * @param datesize Max size of date
 * @param hour The string that will contain the formated hour
 * @param hoursize Max size of hour
 * @return 0 If both strings have been filled
 */
int time_system_convert_time(uint64_t unix_usec, int32_t minuteswest,
		char *date, size_t datesize, char *hour, size_t hoursize);

/**
 * @brief Compute a subtraction of two timespec values.
 *
 * @param start timespec start time
 * @param end timespec end time
 * @param diff pointer of result (diff = end - start)
 * @return 0 on success, negative errno value on errors
 */
int time_timespec_diff(const struct timespec *start,
		       const struct timespec *end,
		       struct timespec *diff);

/**
 * @brief Check if the diff of two timespec values is in a specific range.
 *
 * @param t1 first timespec
 * @param t2 first timespec
 * @param range_us the range in microseconds
 * @param diff_us the diff between the two timespec. Can be NULL
 * @return 1 if the diff is in the range, else 0
 */
int time_timespec_diff_in_range(const struct timespec *t1,
				const struct timespec *t2,
				uint64_t range_us,
				uint64_t *diff_us);

/**
 * @brief Convert a timespec value to uint64_t in usec.
 *
 * @param value timespec value
 * @param us pointer to usec filled by function on success
 * @return 0 on success, negative errno value on errors
 */
int time_timespec_to_us(const struct timespec *value, uint64_t *us);

/**
 * @brief Convert a uint64_t value in usec into a timespec value.
 *
 * @param value uint64_t value, measured in us
 * @param ts pointer to timespec structure filled by function on success
 * @return 0 on success, negative errno value on errors
 */
int time_us_to_timespec(const uint64_t *value, struct timespec *ts);

/**
 * @brief Convert a uint64_t value in nsec into a timespec value.
 *
 * @param value uint64_t value, measured in ns
 * @param ts pointer to timespec structure filled by function on success
 * @return 0 on success, negative errno value on errors
 */
int time_ns_to_timespec(const uint64_t *value, struct timespec *ts);

/**
 * @brief Compute elapse time from timespec value & now
 * result hold in timespec value.
 *
 * @param value timespec value
 * @param diff pointer to elapse time filled by function on success
 * @return 0 on success, negative errno value on errors
 */
int time_timespec_diff_now(const struct timespec *value, struct timespec *diff);

/**
 * @brief Compare two timespec values
 *
 * @param t1 pointer to first timespec value (must be non NULL)
 * @param t2 pointer to second timespec value (must be non NULL)
 *
 * @return 0 if the two values are equal, or if an argument is NULL
 *         1 if t1 is after t2
 *         -1 if t1 is before t2
 */
int time_timespec_cmp(const struct timespec *t1, const struct timespec *t2);

/**
 * @brief Add (or substract if delta < 0) to a timespec.
 * @param ts : source timespec.
 * @param delta : value to add (or substract if delta < 0) in us.
 * @param res: result value.
 * @return 0 on success, negative errno value on errors
 * @remarks: behaviour is undefined if operation causes overlap of the tv_sec
 * field.
 */
int time_timespec_add_us(const struct timespec *ts, int64_t delta, struct timespec *res);

/**
 * @brief Add (or substract if delta < 0) to a timespec.
 * @param ts : source timespec.
 * @param delta : value to add (or substract if delta < 0) in ns.
 * @param res: result value.
 * @return 0 on success, negative errno value on errors
 * @remarks: behaviour is undefined if operation causes overlap of the tv_sec
 * field.
 */
int time_timespec_add_ns(const struct timespec *ts, int64_t delta, struct timespec *res);

/**
 * @brief Convert a timeval to a timespec
 * @param tv : timeval to convert
 * @param ts : timespec to convert
 * @return 0 on success, negative errno value on errors
 */
int time_timeval_to_timespec(const struct timeval *tv, struct timespec *ts);

/**
 * @brief Convert a timeval value to uint64_t in msec.
 *
 * @param value timeval value
 * @param ms pointer to msec filled by function on success
 * @return 0 on success, negative errno value on errors
 */
int time_timeval_to_ms(const struct timeval *value, uint32_t *ms);

#ifdef __cplusplus
}
#endif

#endif /* _TIMETOOLS_H_ */

