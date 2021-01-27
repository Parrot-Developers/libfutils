/******************************************************************************
 * Copyright (c) 2021 Parrot S.A.
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
 * @file safew.c
 *
 * @brief safe write file system functions
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "safew_types.h"
#include "futils/safew.h"

#define ULOG_TAG futils_safew
#include <ulog.h>
ULOG_DECLARE_TAG(ULOG_TAG);

struct futils_safew_file *futils_safew_fopen(const char *pathname)
{
	int ret;
	struct futils_safew_file *safew_fp;

	safew_fp = calloc(1, sizeof(struct futils_safew_file));
	if (!safew_fp)
		return NULL;

	ret = snprintf(safew_fp->tmp_path, sizeof(safew_fp->tmp_path),
		       "%s%s", pathname, SAFEW_TMP_SUFFIX);
	if (ret < 0 || ret >= (int)sizeof(safew_fp->tmp_path)) {
		free(safew_fp);
		return NULL;
	}
	ret = snprintf(safew_fp->path, sizeof(safew_fp->path), "%s", pathname);
	if (ret < 0 || ret >= (int)sizeof(safew_fp->path)) {
		free(safew_fp);
		return NULL;
	}

	/* remove any existing tmp file that would be previous failure */
	if (unlink(safew_fp->tmp_path) == 0)
		ULOGI("removed previous safew tmp file '%s'",
		      safew_fp->tmp_path);

	ULOGD("safe write open file %s", safew_fp->path);
	safew_fp->fp = fopen(safew_fp->tmp_path, "w");
	if (safew_fp->fp == NULL) {
		free(safew_fp);
		return NULL;
	}
	safew_fp->failure = 0;

	return safew_fp;
}

int futils_safew_fclose_rollback(struct futils_safew_file *safew_fp)
{
	int ret = 0;

	if (safew_fp == NULL)
		return 0;
	ULOGD("safe write close rollback %s", safew_fp->path);

	if (fclose(safew_fp->fp))
		ret = -1;

	if (unlink(safew_fp->tmp_path))
		ret = -1;

	free(safew_fp);

	return ret;
}

int futils_safew_fclose_commit(struct futils_safew_file *safew_fp)
{
	int ret = 0;

	if (safew_fp == NULL)
		return 0;
	ULOGD("safe write close %s", safew_fp->path);

	/* synchronize file's in-core state with storage device */
	if (fflush(safew_fp->fp))
		safew_fp->failure = 1;
	/* threadx close is doing a sync
	 * fsync() is not supported by Mingw
	 */
#if !defined(_WIN32)
# if !defined(THREADX_OS)
	if (fsync(fileno(safew_fp->fp)))
		safew_fp->failure = 1;
# endif
#else
# warning "fsync is not supported"
#endif
	if (fclose(safew_fp->fp))
		safew_fp->failure = 1;

	if (safew_fp->failure) {
		ULOGE("safe write %s: error", safew_fp->path);
		ret = unlink(safew_fp->tmp_path);
	} else {
		/* Ensure the update is atomic, in order to prevent partial
		 * write. With this rename, the file is either previous one,
		 * either new one once completely written. */
		ret = rename(safew_fp->tmp_path, safew_fp->path);
		if (ret == -1)
			unlink(safew_fp->tmp_path);
	}

	free(safew_fp);

	return ret == 0 ? 0 : -1;
}

int futils_safew_fprintf(struct futils_safew_file *safew_fp,
			 const char *format, ...)
{
#ifndef THREADX_OS
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = vfprintf(safew_fp->fp, format, ap);
	va_end(ap);
	if (ret < 0)
		safew_fp->failure = 1;

	return ret;
#else
	/* threadx vfprintf doesn't work properly, use vsnprintf instead */
	char buffer[SAFEW_BUFFER_SIZE];
	va_list ap;
	int size;
	size_t wr_size;

	va_start(ap, format);
	size = vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);
	if (size < 0 || size >= (int)sizeof(buffer)) {
		ULOGE("Write error for '%s' in %zu bytes buffer",
		      format, sizeof(buffer));
		safew_fp->failure = 1;
		return -1;
	}
	wr_size = fwrite(buffer, 1, size, safew_fp->fp);
	if (wr_size != size)
		safew_fp->failure = 1;

	return (int)wr_size;
#endif
}

size_t futils_safew_fwrite(const void *ptr, size_t size, size_t nmemb,
			   struct futils_safew_file *safew_fp)
{
	size_t ret;

	ret = fwrite(ptr, size, nmemb, safew_fp->fp);
	if (ret != nmemb)
		safew_fp->failure = 1;

	return ret;
}
