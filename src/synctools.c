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
 * @file synctools.c
 *
 * @brief Synchronize files and folders on file system
 *
 ******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "futils/synctools.h"

#define ULOG_TAG synctools
#include <ulog.h>
ULOG_DECLARE_TAG(synctools);

/**
 * @brief Synchronize a file or a folder
 *
 * @param[in] path The target file/folder
 * @param[in] oflags The additionnal flags to use with open
 *
 * @return 0 on success
 *         negative errno on error
 */
static int synctool(const char *path, int oflags)
{
	int fd;
	int ret;

	if (!path)
		return -EINVAL;

	/* Write new data */
	fd = open(path, O_RDONLY | oflags);
	if (fd == -1) {
		ret = -errno;
		ULOGE("open(%s) failed : %d(%m)", path, -ret);
		return ret;
	}

	ret = fsync(fd);
	if (ret == -1) {
		ret = -errno;
		ULOGE("fsync(%s) failed : %d(%m)", path, -ret);
	}

	close(fd);

	return ret;
}

int sync_file(const char *filepath)
{
	return synctool(filepath, 0);
}

int sync_folder(const char *folderpath)
{
	return synctool(folderpath, O_DIRECTORY);
}

int sync_file_and_folder(const char *filepath)
{
	int ret;
	char *slash;
	char *folderpath;

	/* sync file */
	ret = sync_file(filepath);
	if (ret < 0)
		ULOGW("sync_file(%s) failed : %d(%s)", filepath, ret,
				strerror(-ret));

	/* determine parent folder */
	folderpath = strdup(filepath);
	if (!folderpath) {
		ULOGE("Could not duplicate path");
		return -ENOMEM;
	}

	slash = strrchr(folderpath, '/');
	if (!slash) {
		ULOGE("Could not get parent folder of %s", filepath);
		ret = -EINVAL;
		goto out;
	}
	slash[1] = '\0';

	/* sync parent folder */
	ret = sync_folder(folderpath);
	if (ret < 0)
		ULOGW("sync_folder(%s) failed : %d(%s)", folderpath, ret,
				strerror(-ret));

out:
	free(folderpath);
	return ret;
}
