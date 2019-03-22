/******************************************************************************
 * Copyright (c) 2019 Parrot S.A.
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
 * @file fs.c
 *
 * @brief file system utility tools
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include "futils/fs.h"

#define ULOG_TAG futils_fs
#include <ulog.h>
ULOG_DECLARE_TAG(ULOG_TAG);

int64_t futils_fs_dir_size(const char *path, bool recursive)
{
	DIR *dir;
	int64_t ret = 0;
	int64_t size = 0;
	struct dirent *entry = NULL;
	char *filepath = NULL;
	struct stat stats;

	/* open dir */
	dir = opendir(path);
	if (!dir) {
		ret = -errno;
		ULOG_ERRNO("can't opendir '%s'", errno, path);
		goto clean;
	}

	filepath = malloc(FILENAME_MAX);
	if (!filepath) {
		ret = -ENOMEM;
		goto clean;
	}

	/* iterate from dir */
	while ((entry = readdir(dir)) != NULL) {
		/* skip "." and ".." */
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		/* get entry status */
		snprintf(filepath, FILENAME_MAX, "%s/%s", path, entry->d_name);
		if (lstat(filepath, &stats) < 0) {
			ret = -errno;
			ULOG_ERRNO("can't lstat '%s'", errno, filepath);
			goto clean;
		}

		/* add regular file size */
		if (S_ISREG(stats.st_mode)) {
			size += stats.st_size;
			continue;
		}

		/* if recursive is true add sub-directories size */
		if (S_ISDIR(stats.st_mode) && recursive) {
			ret = futils_fs_dir_size(filepath, true);
			if (ret < 0)
				goto clean;

			size += ret;
		}
	}

clean:
	free(filepath);
	if (dir)
		closedir(dir);

	if (ret < 0)
		return ret;
	else
		return size;
}
