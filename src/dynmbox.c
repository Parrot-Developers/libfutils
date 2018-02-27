/******************************************************************************
 * Copyright (c) 2017 Parrot S.A.
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
 * @file dynmbox.c
 *
 * @brief One-to-one non-blocking mailbox mechanism for messages of varying size
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __USE_GNU
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <sys/uio.h>

#define ULOG_TAG dynmbox
#include <ulog.h>
ULOG_DECLARE_TAG(dynmbox);

#include "futils/fdutils.h"
#include "futils/dynmbox.h"

struct dynmbox {
	/* pipes fds */
	int fds[2];
	/* message size */
	size_t max_msg_size;
	/* Actual pipe capacity */
	ssize_t pipe_capacity;
};

static ssize_t read_no_eintr(int fd, void *buf, size_t count)
{
	ssize_t ret;

	do {
		ret = read(fd, buf, count);
	} while (ret == -1 && errno == EINTR);

	/* check eof */
	if (ret == 0)
		return -EPIPE;

	return ret >= 0 ? ret : -errno;
}

struct dynmbox *dynmbox_new(size_t max_msg_size)
{
	struct dynmbox *box;
	int i, ret;

	/* allocate box */
	box = calloc(1, sizeof(*box));
	if (!box)
		return NULL;

	/* create pipes */
	ret = pipe(box->fds);
	if (ret < 0) {
		free(box);
		return NULL;
	}

	box->pipe_capacity = fcntl(box->fds[0], F_GETPIPE_SZ, 0);
	if (box->pipe_capacity  < 0) {
		close(box->fds[0]);
		close(box->fds[1]);
		free(box);
		return NULL;
	}

	/* Check the size of the message plus its header against the genuine
	 * capacity of the pipe */
	if (max_msg_size + sizeof(size_t) > (size_t) box->pipe_capacity) {
		close(box->fds[0]);
		close(box->fds[1]);
		free(box);
		return NULL;
	}

	/* set pipes non blocking and CLOEXEC flag */
	for (i = 0; i < 2; i++) {
		fd_add_flags(box->fds[i], O_NONBLOCK);
		fd_set_close_on_exec(box->fds[i]);
	}

	box->max_msg_size = max_msg_size;
	return box;
}

void dynmbox_destroy(struct dynmbox *box)
{
	if (!box)
		return;

	close(box->fds[0]);
	close(box->fds[1]);
	free(box);
}

int dynmbox_get_read_fd(const struct dynmbox *box)
{
	return box ? box->fds[0] : -1;
}

ssize_t dynmbox_get_max_size(const struct dynmbox *box)
{
	return box ? (ssize_t) box->max_msg_size : -EINVAL;
}

int dynmbox_push(struct dynmbox *box,
			 const void *msg,
			 size_t msg_size)
{
	int ret;
	int bytes_in_pipe;
	/* The message is in two parts : an header containing its size, and
	 * the payload */
	struct iovec iov[2] = {
		{ .iov_base = &msg_size,   .iov_len = sizeof(msg_size) },
		{ .iov_base = (void *) msg, .iov_len = msg_size }
	};

	if (!box || msg_size > box->max_msg_size || (msg_size > 0 && !msg))
		return -EINVAL;

	ret = ioctl(box->fds[1], FIONREAD, &bytes_in_pipe);
	if (ret < 0)
		return -errno;

	/* The cast is safe because if the ioctl returned with a non-negative
	 * value, there is less bytes currently in the pipe than its
	 * capacity */
	if ((size_t) (box->pipe_capacity - bytes_in_pipe) < msg_size
							    + sizeof(msg_size))
		return -EAGAIN;

	do {
		ret = writev(box->fds[1], iov, (msg_size == 0) ? 1 : 2);
	} while (ret == -1 && errno == EINTR);

	return ret == (int) (msg_size + sizeof(size_t)) ? 0 : -EAGAIN;
}

ssize_t dynmbox_peek(struct dynmbox *box, void *msg)
{
	size_t to_read;
	ssize_t ret;

	if (!msg || !box)
		return -EINVAL;

	/* Read the message header */
	ret = read_no_eintr(box->fds[0], &to_read, sizeof(to_read));

	if (ret < 0)
		return ret;

	if (to_read == 0) {
		/* message size is 0, do not read further */
		return 0;
	}
	/* Read the whole message */
	return read_no_eintr(box->fds[0], msg, to_read);
}
