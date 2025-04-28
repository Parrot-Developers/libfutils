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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "futils/safew.h"
#include "safew_types.h"
#define ULOG_TAG futils_safew
#include <ulog.h>
ULOG_DECLARE_TAG(ULOG_TAG);

#define PAYLOAD  (1 << 0)
#define CRC  (1 << 1)
#define TMP_PAYLOAD (1 << 2)
#define TMP_CRC (1 << 3)

static void crc_from_fhd(FILE *fhd, uint32_t *crc)
{
	int val;

	*crc = 0;
	for (val = getc(fhd); val != EOF; val = getc(fhd)) {
		*crc += val;
		*crc += *crc << 10;
		*crc ^= *crc >> 6;
	}

	*crc += *crc << 3;
	*crc ^= *crc >> 11;
	*crc += *crc << 15;
}

static int crc_from_file(const char *pathname, uint32_t *crc)
{
	int ret = 0;
	FILE *fhd = fopen(pathname, "r");

	if (fhd == NULL) {
		ULOGE("can't open file %s", pathname);
		return -1;
	}

	crc_from_fhd(fhd, crc);
	ULOGD("calculated crc:0x%08" PRIX32 " of file %s", *crc, pathname);

	ret = fclose(fhd);
	if (ret < 0)
		return -1;

	return ret;
}

static int read_crc(FILE *fhd, uint32_t *crc)
{
	if (fread(crc, 1, sizeof(uint32_t), fhd) != sizeof(uint32_t)) {
		ULOGE("can't read crc file");
		return -1;
	}

	return 0;
}

static int check_pair(FILE *payload_fhd, FILE *crc_fhd)
{
	int ret = 0;
	uint32_t crc;
	uint32_t payload_crc = 0;

	ret = read_crc(crc_fhd, &crc);
	if (ret < 0)
		return ret;

	crc_from_fhd(payload_fhd, &payload_crc);

	if (crc != payload_crc)
		return -1;

	return 0;
}

static int safew_create_crc_filenames(const char *pathname,
				      struct futils_safew_crc *fp)
{
	int ret;

	if (fp == NULL || pathname == NULL)
		return -1;

	ret = snprintf(fp->path, sizeof(fp->path), "%s%s", pathname,
		       SAFEW_CRC_SUFFIX);
	if (ret < 0 || ret >= (int)sizeof(fp->path))
		return -1;
	ret = snprintf(fp->tmp_path,
		       sizeof(fp->tmp_path),
		       "%s%s", fp->path,
		       SAFEW_TMP_SUFFIX);
	if (ret < 0 || ret >= (int)sizeof(fp->tmp_path))
		return -1;

	return 0;
}

static int safew_tmp_crc_create(const char *pathname, const char *crc_tmp_path)
{
	uint32_t crc = 0;
	FILE *fp;
	int ret;

	/* calculate crc on payload */
	ret = crc_from_file(pathname, &crc);
	if (ret < 0)
		return ret;

	/* write crc to tmp file */
	fp = fopen(crc_tmp_path, "w");
	if (fp == NULL)
		return -1;

	if (fwrite((char *)&crc, 1, sizeof(uint32_t), fp) != sizeof(uint32_t))
		ret = -1;


	if (fflush(fp) < 0)
		ret = -1;
#if !defined(_WIN32)
# if !defined(THREADX_OS)
	if (fsync(fileno(fp)))
		ret = -1;
# endif
#endif

	if (fclose(fp) < 0)
		ret = -1;

	return ret;
}

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

#ifndef __hexagon__
	/* remove any existing tmp file that would be previous failure */
	if (unlink(safew_fp->tmp_path) == 0)
		ULOGI("removed previous safew tmp file '%s'",
		      safew_fp->tmp_path);
#endif

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
		return -1;
	ULOGD("safe write close rollback %s", safew_fp->path);

	if (fclose(safew_fp->fp))
		ret = -1;

	if (unlink(safew_fp->tmp_path))
		ret = -1;

	free(safew_fp);

	return ret;
}

static int safew_fclose_commit(struct futils_safew_file *safew_fp,
			       bool with_crc)
{
	int ret = 0;
	struct futils_safew_crc crc_fp = { {}, {} };

	if (safew_fp == NULL)
		return -1;
	ULOGD("safe write close %s", safew_fp->path);

	if (safew_fp->failure)
		ret = -1;
	else {
		/* synchronize file's in-core state with storage device */
		if (fflush(safew_fp->fp))
			ret = -1;
		/* threadx close is doing a sync
		 * fsync() is not supported by Mingw
		 */
#if !defined(_WIN32)
# if !defined(THREADX_OS)
		if (fsync(fileno(safew_fp->fp)))
			ret = -1;
# endif
#else
# warning "fsync is not supported"
#endif
	}

	/* finish write tmp payload */
	if (fclose(safew_fp->fp))
		ret = -1;

	if (ret == 0 && with_crc) {
		/* create crc tmp */
		ret = safew_create_crc_filenames(safew_fp->path, &crc_fp);
		if (ret == 0)
			ret = safew_tmp_crc_create(safew_fp->tmp_path,
						   crc_fp.tmp_path);
	}

	/* Ensure the update is atomic, in order to prevent partial
	 * write. With this rename, the file is either previous one,
	 * either new one once completely written. */
	if (ret == 0)
		/* move tmp payload to payload */
		ret = rename(safew_fp->tmp_path, safew_fp->path);

	if (ret == 0 && with_crc)
		/* move tmp payload to payload */
		ret = rename(crc_fp.tmp_path, crc_fp.path);

	if (ret < 0) {
		ULOGE("safe write fclose commit %s: error", safew_fp->path);
		unlink(safew_fp->tmp_path);

		if (with_crc) {
			unlink(crc_fp.path);
			unlink(crc_fp.tmp_path);
		}
	}

	free(safew_fp);

	return ret == 0 ? 0 : -1;
}

int futils_safew_fclose_commit(struct futils_safew_file *safew_fp)
{
	return safew_fclose_commit(safew_fp, false);
}

int futils_safew_fclose_commit_with_crc(struct futils_safew_file *safew_fp)
{
	return safew_fclose_commit(safew_fp, true);
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

int futils_safew_file_check(const char *pathname)
{
	struct futils_safew_file fp;
	struct futils_safew_crc crc_fp;
	uint32_t crc, tmp_crc;
	uint32_t payload_crc = 0;
	uint32_t tmp_payload_crc = 0;
	FILE *payload_fhd;
	FILE *crc_fhd;
	FILE *tmp_payload_fhd;
	FILE *tmp_crc_fhd;

	int ret;

	/* create all associated filenames */
	ret = snprintf(fp.tmp_path, sizeof(fp.tmp_path),
		       "%s%s", pathname, SAFEW_TMP_SUFFIX);
	if (ret < 0 || ret >= (int)sizeof(fp.tmp_path))
		return -1;
	ret = safew_create_crc_filenames(pathname, &crc_fp);
	if (ret < 0)
		return -1;

	/* try to open all files */
	payload_fhd = fopen(pathname, "r");
	tmp_payload_fhd = fopen(fp.tmp_path, "r");
	crc_fhd = fopen(crc_fp.path, "r");
	tmp_crc_fhd = fopen(crc_fp.tmp_path, "r");

	/* create an error code with existing files */
	uint8_t present_files =
		((payload_fhd != NULL) * PAYLOAD) +
		((crc_fhd != NULL) * CRC) +
		((tmp_payload_fhd != NULL) * TMP_PAYLOAD) +
		((tmp_crc_fhd != NULL) * TMP_CRC);

	/* the check and recovery of payload/crc pairs rely on the
	 * order of file's manipulations.
	 *
	 * payload and crc does not exist:
	 *
	 * stage 1 - tmp payload is created:
	 *	tmp_payload_v1
	 * stage 2 - tmp crc is calculated:
	 *	tmp_payload_v1 + tmp_crc_v1
	 * stage 3 - tmp payload is moved to payload:
	 *	payload_v1 + tmp_crc_v1
	 * stage 4 - tmp crc is moved crc:
	 *	payload_v1 + crc_v1
	 *
	 * new payload:
	 *
	 * stage 6 - tmp payload is created:
	 *	payload_v1 + crc_v1 + tmp_payload_v2
	 * stage 7 - tmp crc is created:
	 *	payload_v1 + crc_v1 + tmp_payload_v2 + tmp_crc_v2
	 * stage 8 - tmp payload is moved to payload:
	 *	payload_v2 + crc_v1 + tmp_crc_v2
	 * stage 9 - tmp crc is moved to crc:
	 *	payload_v2 + crc_v2
	 */

	switch (present_files) {
	case 0:		/* none is present */
	case PAYLOAD:
	case TMP_PAYLOAD:
	case PAYLOAD + TMP_PAYLOAD:
	case CRC:
	case TMP_CRC:
	case CRC + TMP_CRC:
		/* nothing to match */
		ret = -1;
		break;

	case TMP_PAYLOAD + CRC:
		/* as the tmp payload is created before the tmp crc; this crc
		 * is the crc of a previous payload and can't be matched
		 * with tmp payload */
		ret = -1;
		break;

	case PAYLOAD + CRC:
	case PAYLOAD + CRC + TMP_PAYLOAD:
		/* check if crc file matches the payload; if tmp payload is
		 * also present, it can't be used as crc is then the crc of
		 * the previous payload */
		ret = check_pair(payload_fhd, crc_fhd);
		break;

	case PAYLOAD + TMP_CRC:
		/* this looks like a stage 3 */
		ret = check_pair(payload_fhd, tmp_crc_fhd);
		if (ret < 0)
			break;

		/* rename tmp crc file */
		ret = rename(crc_fp.tmp_path, crc_fp.path);
		break;

	case TMP_PAYLOAD + TMP_CRC:
		/* this looks like a stage 2 */
		ret = check_pair(tmp_payload_fhd, tmp_crc_fhd);
		if (ret < 0)
			break;

		/* rename tmp payload and crc */
		ret = rename(fp.tmp_path, pathname);
		if (ret < 0)
			break;
		ret = rename(crc_fp.tmp_path, crc_fp.path);
		break;

	case PAYLOAD + CRC + TMP_CRC:
		/* this could be stage 7 with tmp_payload_v2 disappeared:
		 * in that case payload could match crc */
		ret = read_crc(crc_fhd, &crc);
		if (ret < 0)
			break;

		crc_from_fhd(payload_fhd, &payload_crc);

		/* if pair matches let's stop here */
		if (crc == payload_crc)
			break;

		/* this could be stage 8, in that case payload should match
		 * tmp crc */
		ret = read_crc(tmp_crc_fhd, &crc);
		if (ret < 0)
			break;

		if (crc == payload_crc) {
			/* rename tmp crc */
			ret = rename(crc_fp.tmp_path, crc_fp.path);
			break;
		}

		ULOGE("no matching crc found for %s", pathname);
		ret = -1;
		break;

	case PAYLOAD + TMP_CRC + TMP_PAYLOAD:
		/* this could be stage 7 with crc_v1 disappeared:
		 * in that case tmp_payload should match tmp_crc */
	case TMP_PAYLOAD + CRC + TMP_CRC:
		/* this could be stage 7 with payload_v1 disappeared:
		 * in that case tmp_payload should match tmp_crc */
		ret = read_crc(tmp_crc_fhd, &tmp_crc);
		if (ret < 0)
			break;

		crc_from_fhd(tmp_payload_fhd, &tmp_payload_crc);

		/* if pair doesn't match erase all */
		if (tmp_crc != tmp_payload_crc) {
			ULOGE("no matching crc found for %s", pathname);
			ret = -1;
			break;
		}

		/* rename tmp crc and tmp payload */
		ret = rename(crc_fp.tmp_path, crc_fp.path);
		if (ret < 0)
			break;
		ret = rename(fp.tmp_path, pathname);
		break;

	case PAYLOAD + TMP_PAYLOAD + CRC + TMP_CRC:
		/* stage 7: payload/crc and tmp_payload/tmp_crc pairs should be
		 * both valid.
		 * Check tmp pair first, if it fails test the other one
		 */
		ret = read_crc(tmp_crc_fhd, &tmp_crc);
		if (ret < 0)
			break;
		crc_from_fhd(tmp_payload_fhd, &tmp_payload_crc);

		if (tmp_crc == tmp_payload_crc) {
			/* rename tmp crc and tmp payload */
			ret = rename(crc_fp.tmp_path, crc_fp.path);
			if (ret < 0)
				break;
			ret = rename(fp.tmp_path, pathname);
			break;
		}

		/* check the "old" pair (payload + crc) */
		ret = read_crc(crc_fhd, &crc);
		if (ret < 0)
			break;

		crc_from_fhd(payload_fhd, &payload_crc);

		if (crc != payload_crc) {
			ULOGE("no matching crc found for %s", pathname);
			ret = -1;
		}
		break;
	}

	/* close files */
	if (payload_fhd)
		if (fclose(payload_fhd) < 0)
			ULOGE("error closing %s", pathname);
	if (tmp_payload_fhd)
		if (fclose(tmp_payload_fhd) < 0)
			ULOGE("error closing %s", fp.tmp_path);
	if (crc_fhd)
		if (fclose(crc_fhd) < 0)
			ULOGE("error closing %s", crc_fp.path);
	if (tmp_crc_fhd)
		if (fclose(tmp_crc_fhd) < 0)
			ULOGE("error closing %s", crc_fp.tmp_path);

	/* remove remaining tmp file */
	unlink(fp.tmp_path);
	unlink(crc_fp.tmp_path);

	/* in case of error remove all files */
	if (ret < 0) {
		ULOGE("no valid crc could be found for: %s", pathname);
		unlink(pathname);
		unlink(crc_fp.path);
	}

	return ret;
}
