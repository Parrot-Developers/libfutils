/**
 * @file synctools.c
 *
 * @author eric.brunet@parrot.com
 *
 * @copyright Copyright (C) 2016 Parrot S.A.
 */

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
