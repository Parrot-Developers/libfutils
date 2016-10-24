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
 * @file mbox.c
 *
 * @brief Simple mailbox mechanism guaranteeing atomic read/write
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "futils/mbox.h"

struct mbox {
	/* pipes fds */
	int fds[2];
	/* message size */
	size_t msg_size;
};

static int set_close_on_exec(int fd)
{
	int old, ret;

	old = fcntl(fd, F_GETFD, 0);
	if (old < 0)
		return -errno;

	ret = fcntl(fd, F_SETFD, FD_CLOEXEC | old);
	if (ret < 0)
		return -errno;

	return 0;
}


static int add_fd_flags(int fd, int flags)
{
	int old, ret;

	old = fcntl(fd, F_GETFL, 0);
	if (old < 0)
		return -errno;

	ret = fcntl(fd, F_SETFL, old | flags);
	if (ret < 0)
		return -errno;

	return 0;
}

struct mbox *mbox_new(size_t msg_size)
{
	struct mbox *box;
	int i, ret;

	/* message size must be lesser than PIPE_BUF (for atomic write) */
	if (msg_size == 0 || msg_size >= PIPE_BUF)
		return NULL;

	/* allocate mbox */
	box = calloc(1, sizeof(*box));
	if (!box)
		return NULL;

	/* create pipes */
	ret = pipe(box->fds);
	if (ret < 0) {
		free(box);
		return NULL;
	}

	/* set pipes non blocking and CLOEXEC flag */
	for (i = 0; i < 2; i++) {
		add_fd_flags(box->fds[i], O_NONBLOCK);
		set_close_on_exec(box->fds[i]);
	}

	box->msg_size = msg_size;
	return box;
}

void mbox_destroy(struct mbox *box)
{
	if (!box)
		return;

	close(box->fds[0]);
	close(box->fds[1]);
	free(box);
}

int mbox_get_read_fd(const struct mbox *box)
{
	return box ? box->fds[0] : -1;
}

int mbox_push(struct mbox *box, const void *msg)
{
	ssize_t ret;

	if (!msg || !box)
		return -EINVAL;

	/* write without blocking */
	do {
		ret = write(box->fds[1], msg, box->msg_size);
	} while (ret == -1 && errno == EINTR);

	return (ret < 0) ? -errno : 0;
}

int mbox_peek(struct mbox *box, void *msg)
{
	ssize_t ret;

	if (!msg || !box)
		return -EINVAL;

	/* read without blocking */
	do {
		ret = read(box->fds[0], msg, box->msg_size);
	} while (ret == -1 && errno == EINTR);

	/* check eof */
	if (ret == 0)
		return -EPIPE;

	return (ret < 0) ? -errno : 0;
}

