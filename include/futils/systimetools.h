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
 * @file systimetools.h
 *
 * @brief drone system time tools.
 *
 ******************************************************************************/

#ifndef _SYSTIMETOOLS_H_
#define _SYSTIMETOOLS_H_

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
		int32_t *minuteswest);

/**
 * System time API
 */

/**
 * @brief Set the time of the system
 *
 * @param unix_usec Timestamp of the master clock in microseconds since UNIX
 *                  epoch.
 * @param minuteswest minutes west of Greenwich.
 *
 * @return 0 on success
 */
int time_system_set_time(uint64_t unix_usec, int32_t minuteswest);

/**
 * @brief Get the time of the system
 *
 * @param unix_usec Timestamp of the master clock in microseconds since UNIX
 *                  epoch.
 * @param minuteswest minutes west of Greenwich.
 *
 * @return 0 on success
 * @return -EINVAL if at last one parameter is NULL
 */
int time_system_get_time(uint64_t *unix_usec, int32_t *minuteswest);

/**
 * @brief Fill a tm struct based on unix_usec & minuteswest
 *
 * @param unix_usec Timestamp of the master clock in microseconds since UNIX
 *                  epoch.
 * @param minuteswest minutes west of Greenwich.
 *
 * @return 0 on success
 * @return -EINVAL if tm is NULL
 */
int time_system_create_tm(uint64_t unix_usec, int32_t minuteswest,
		struct tm *tm);

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

#ifdef __cplusplus
}
#endif

#endif /* _SYSTIMETOOLS_H_ */
