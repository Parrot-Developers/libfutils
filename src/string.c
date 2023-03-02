/******************************************************************************
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
 * @file string.c
 *
 * @brief string utility functions
 *
 ******************************************************************************/

#include "futils/string.h"

#include <string.h>
#include <errno.h>
#include <iconv.h>
#include <wchar.h>

#define ULOG_TAG futils_string
#include <ulog.h>
ULOG_DECLARE_TAG(ULOG_TAG);


static int to_wchar_string(const char *input_, size_t in_len, wchar_t **output_)
{
	char *input, *output;
	size_t inbytes, outbytes, outsize, sret;
	wchar_t *woutput = NULL;
	iconv_t conv;
	int ret = 0;

	*output_ = NULL;

	conv = iconv_open("WCHAR_T", "UTF-8");
	if (conv == (iconv_t)-1) {
		ULOGD("iconv(WCHAR_T, UTF-8) is not available : %s(%d)",
		      strerror(errno),
		      errno);
		return -ENOSYS;
	}

	woutput = calloc(in_len + 1, sizeof(wchar_t));
	if (!woutput) {
		ret = -ENOMEM;
		goto exit;
	}

	input = (char *)input_;
	inbytes = in_len;
	output = (char *)woutput;
	outsize = in_len * sizeof(wchar_t);
	outbytes = outsize;
	sret = iconv(conv, &input, &inbytes, &output, &outbytes);
	if (sret == (size_t)-1) {
		ret = -errno;
		ULOGD("iconv failed with error %s(%d)", strerror(-ret), -ret);
		goto exit;
	}

	/* ret is the number of wide characters written to output_ */
	ret = (outsize - outbytes) / sizeof(wchar_t);


exit:
	iconv_close(conv);
	if (ret < 0)
		free(woutput);
	else
		*output_ = woutput;
	return ret;
}


static int to_utf8_string(wchar_t *input_, char *output_, size_t max_output_len)
{
	char *input, *output;
	size_t inbytes, outbytes, outsize, sret;
	iconv_t conv;
	int ret = 0;

	conv = iconv_open("UTF-8", "WCHAR_T");
	if (conv == (iconv_t)-1) {
		ULOGD("iconv(UTF-8, WCHAR_T) is not available : %s(%d)",
		      strerror(errno),
		      errno);
		return -ENOSYS;
	}

	input = (char *)input_;
	inbytes = wcslen(input_) * sizeof(wchar_t);
	output = (char *)output_;
	outsize = max_output_len - 1;
	outbytes = outsize;
	sret = iconv(conv, &input, &inbytes, &output, &outbytes);
	if (sret == (size_t)-1 && errno != E2BIG) {
		/* E2BIG error is expected here as we want to truncate the
		 * string to max_output_len bytes */
		ret = -errno;
		ULOGD("iconv failed with error %s(%d)", strerror(-ret), -ret);
		goto exit;
	}

	/* ret is the number of characters written to output_ */
	ret = outsize - outbytes;
	output_[ret] = '\0';


exit:
	iconv_close(conv);
	return ret;
}


int futils_string_sanitize_utf8(const char *input,
				char *output,
				size_t max_output_size,
				const wchar_t *forbidden_chars,
				wchar_t replacement_char)
{
	int ret;
	size_t in_len, w_len, i;
	wchar_t *winput = NULL;

	ULOG_ERRNO_RETURN_ERR_IF(!input, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(!output, EINVAL);
	ULOG_ERRNO_RETURN_ERR_IF(max_output_size < 2, EINVAL);

	/* Convert to wchar_t */
	in_len = strlen(input);
	ret = to_wchar_string(input, in_len, &winput);
	if (ret < 0)
		goto exit;
	w_len = ret;

	/* Replace forbidden chars */
	if (!forbidden_chars)
		goto cut;
	for (i = 0; i < w_len; i++) {
		if (wcschr(forbidden_chars, winput[i]) == NULL)
			continue;
		winput[i] = replacement_char;
	}

cut:
	/* Convert back to utf-8 */
	ret = to_utf8_string(winput, output, max_output_size);
	if (ret != 0)
		goto exit;

exit:
	free(winput);
	return ret;
}

int futils_string_check_utf8(const char *input,
			     size_t max_bytes,
			     size_t max_wchars,
			     const wchar_t *forbidden_chars)
{
	int ret;
	size_t in_len, w_len, i;
	wchar_t *winput = NULL;

	ULOG_ERRNO_RETURN_ERR_IF(!input, EINVAL);

	/* Check for max_bytes */
	in_len = strlen(input);
	if (max_bytes > 0 && in_len >= max_bytes) {
		ret = -E2BIG;
		goto exit;
	}

	/* Convert to wchar_t */
	ret = to_wchar_string(input, in_len, &winput);
	if (ret < 0)
		goto exit;
	w_len = ret;
	ret = 0;

	/* Check for max_wchars */
	if (max_wchars > 0 && w_len > max_wchars) {
		ret = -E2BIG;
		goto exit;
	}

	/* Check for forbidden chars */
	if (!forbidden_chars)
		goto exit;
	for (i = 0; i < w_len; i++) {
		if (wcschr(forbidden_chars, winput[i]) == NULL)
			continue;
		ret = -EPROTO;
		goto exit;
	}

exit:
	free(winput);
	return ret;
}
