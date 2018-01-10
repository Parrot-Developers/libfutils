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
 * @file inotify.c
 *
 * @brief inotify wrapper to ease its usage in various Parrot tools
 *
 ******************************************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "futils/inotify.h"
#include "futils/fdutils.h"

#define ULOG_TAG inotify
#include <ulog.h>
ULOG_DECLARE_TAG(inotify);

int inotify_create(const char *path, uint32_t mask)
{
	int fd, wd;

	fd = inotify_init();
	if (fd < 0) {
		ULOGE("inotify_init1: %s\n", strerror(errno));
		goto fail;
	}
	(void)fd_set_close_on_exec(fd);

	wd = inotify_add_watch(fd, path, mask);
	if (wd < 0) {
		ULOGE("inotify_add_watch(%s): %s\n", path, strerror(errno));
		goto fail;
	}

	return fd;
fail:
	inotify_destroy(fd);
	return -1;
}

void inotify_destroy(int fd)
{
	if (fd >= 0)
		close(fd);
}

void inotify_process_fd(int fd, inotify_cb_t cb,
			void *data)
{
	void *evbuf;
	int ret, count, len;
	struct inotify_event *ev;
	/* codecheck_ignore[PREFER_ALIGNED] */
	char buf[512] __attribute__((aligned(sizeof(long long))));

	/* retrieve the number of bytes we should read */
	ret = ioctl(fd, FIONREAD, &count);
	if ((ret < 0) || (count <= 0))
		return;

	/* if there is too much data to read, fallback on heap */
	evbuf = (count > (int)sizeof(buf)) ? malloc(count) : buf;
	if (evbuf == NULL)
		return;

	ev = evbuf;
	ret = read(fd, evbuf, count);

	if (ret == count) {
		while (count >= (int)sizeof(*ev)) {
			(*cb)(ev, data);
			len = sizeof(*ev) + ev->len;
			ev = (struct inotify_event *)((char *)ev + len);
			count -= len;
		}
	}

	if (evbuf != buf)
		free(evbuf);
}
