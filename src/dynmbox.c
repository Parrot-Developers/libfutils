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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <winsock2.h>
#else /* !_WIN32 */
# define __USE_GNU
# include <fcntl.h>
# include <unistd.h>
# include <sys/ioctl.h>
# include "futils/fdutils.h"
#endif

#define ULOG_TAG dynmbox
#include <ulog.h>
ULOG_DECLARE_TAG(dynmbox);

#include "futils/dynmbox.h"
#include "futils/timetools.h"

#define ALLOCATED_LEN (DYNMBOX_MAX_SIZE + sizeof(uint32_t))

struct dynmbox {
#ifdef _WIN32
	/* socket fds */
	SOCKET server;
	SOCKET rfd;
	SOCKET wfd;
#else
	/* pipes fds */
	int fds[2];
#endif
	/* message size */
	size_t max_msg_size;
	/* Memory allocated for buffers */
	uint8_t *bufmem;
	size_t write_idx;
	size_t read_idx;
	size_t used;
	pthread_mutex_t lock;
	pthread_cond_t cond;
};

#ifdef _WIN32
static int wsa2errno(void)
{
	int err = WSAGetLastError();
	int no;
	/* We could just return with the value, but codecheck dislikes it */
	switch (err) {
	case WSAEINTR:
		no = EINTR;
		break;
	case WSAEBADF:
		no = EBADF;
		break;
	case WSAEACCES:
		no = EACCES;
		break;
	case WSAEINVAL:
		no = EINVAL;
		break;
	case WSAEMFILE:
		no = EMFILE;
		break;
	case WSAEWOULDBLOCK:
		no = EWOULDBLOCK;
		break;
	case WSAEALREADY:
		no = EALREADY;
		break;
	case WSAEADDRINUSE:
		no = EADDRINUSE;
		break;
	case WSAECONNRESET:
		no = ECONNRESET;
		break;
	case WSAETIMEDOUT:
		no = ETIMEDOUT;
		break;
	case WSANOTINITIALISED:
		no = ENOSYS;
		break;
	default:
		no = EIO;
		break;
	}
	return no;
}

static void destroy_notify_channel(struct dynmbox *box)
{
	if (box->server != INVALID_SOCKET) {
		shutdown(box->server, SD_BOTH);
		closesocket(box->server);
	}

	if (box->rfd != INVALID_SOCKET) {
		shutdown(box->rfd, SD_BOTH);
		closesocket(box->rfd);
	}

	if (box->wfd != INVALID_SOCKET) {
		shutdown(box->wfd, SD_BOTH);
		closesocket(box->wfd);
	}
}

static int init_notify_channel(struct dynmbox *box)
{
	int res;
	struct sockaddr_in addr;
	int addrlen = 0;
	BOOL nodelay = TRUE;
	u_long mode;

	box->server = INVALID_SOCKET;
	box->rfd = INVALID_SOCKET;
	box->wfd = INVALID_SOCKET;

	box->server = socket(AF_INET, SOCK_STREAM, 0);
	if (box->server == INVALID_SOCKET) {
		res = -wsa2errno();
		ULOG_ERRNO("socket", -res);
		goto error;
	}

	/* Bind and listen the server socket (on a dynamic port) */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(box->server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		res = -wsa2errno();
		ULOG_ERRNO("bind", -res);
		goto error;
	}

	if (listen(box->server, 1) < 0) {
		res = -wsa2errno();
		ULOG_ERRNO("listen", -res);
		goto error;
	}

	/* Retreive the bound port */
	addrlen = sizeof(addr);
	if (getsockname(box->server, (struct sockaddr *)&addr, &addrlen) < 0) {
		res = -wsa2errno();
		ULOG_ERRNO("getsockname", -res);
		goto error;
	}

	/* Create a write fd and connect to server  */
	box->wfd = socket(AF_INET, SOCK_STREAM, 0);
	if (box->wfd == INVALID_SOCKET) {
		res = -wsa2errno();
		ULOG_ERRNO("socket", -res);
		goto error;
	}

	if (connect(box->wfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		res = -wsa2errno();
		ULOG_ERRNO("connect", -res);
		goto error;
	}

	/* Accept connection on server */
	box->rfd = accept(box->server, NULL, NULL);
	if (box->rfd == INVALID_SOCKET) {
		res = -wsa2errno();
		ULOG_ERRNO("accept", -res);
		goto error;
	}

	/* Set non-blocking mode for read end */
	mode = 1;
	if (ioctlsocket(box->rfd, FIONBIO, &mode) < 0) {
		res = -wsa2errno();
		ULOG_ERRNO("ioctlsocket", -res);
		goto error;
	}

	/* Disable Nagle algorithm */
	mode = 1;
	res = setsockopt(box->wfd, IPPROTO_TCP, TCP_NODELAY,
			(const char *)&nodelay, sizeof(nodelay));
	if (res != 0) {
		res = -wsa2errno();
		ULOG_ERRNO("setsockopt", -res);
		goto error;
	}
	return 0;
error:
	destroy_notify_channel(box);
	return res;
}

static void push_notify(struct dynmbox *box)
{
	const char dummy = 0x55;
	if (send(box->wfd, &dummy, sizeof(dummy), 0) < 0)
		ULOG_ERRNO("send() notify", wsa2errno());
}

static void pop_notify(struct dynmbox *box)
{
	char val;
	if (recv(box->rfd, &val, sizeof(val), 0) < 0)
		ULOG_ERRNO("recv() notify", wsa2errno());
}

#else /* !_WIN32 */
static int init_notify_channel(struct dynmbox *box)
{
	int ret;
	int i;

	/* create pipes */
	ret = pipe(box->fds);
	if (ret < 0)
		return -errno;

	/* set pipes CLOEXEC flag, and NONBLOCK for read end */
	for (i = 0; i < 2; i++)
		fd_add_flags(box->fds[i], O_NONBLOCK);
	fd_set_close_on_exec(box->fds[0]);
	return 0;
}

static void destroy_notify_channel(struct dynmbox *box)
{
	close(box->fds[0]);
	close(box->fds[1]);
}

static void push_notify(struct dynmbox *box)
{
	const uint8_t dummy = 0x55;
	ssize_t done;
	done = write(box->fds[1], &dummy, sizeof(dummy));
	if (done == -1)
		ULOG_ERRNO("write() to pipe", errno);
}

static void pop_notify(struct dynmbox *box)
{
	ssize_t done;
	uint8_t val;
	done = read(box->fds[0], &val, sizeof(val));
	if (done == -1)
		ULOG_ERRNO("read() from pipe", errno);
}
#endif

struct dynmbox *dynmbox_new(size_t max_msg_size)
{
	struct dynmbox *box;
	int ret;

	/* Enforce message size limit */
	if (max_msg_size > DYNMBOX_MAX_SIZE)
		return NULL;

	/* allocate box */
	box = calloc(1, sizeof(*box));
	if (!box)
		return NULL;

	/* allocate ring buffer */
	box->bufmem = malloc(ALLOCATED_LEN);
	if (!box->bufmem)
		goto fail_bufmem;

	/* initialize ringbuf state */
	box->read_idx = 0;
	box->write_idx = 0;
	box->used = 0;

	/* create os-specific notification channel */
	ret = init_notify_channel(box);
	if (ret)
		goto fail_notify_channel;

	/* mutex + cond */
	pthread_mutex_init(&box->lock, NULL);
	pthread_cond_init(&box->cond, NULL);

	box->max_msg_size = max_msg_size;
	return box;

fail_notify_channel:
	free(box->bufmem);
fail_bufmem:
	free(box);
	return NULL;
}

void dynmbox_destroy(struct dynmbox *box)
{
	int res;
	if (!box)
		return;

	res = pthread_cond_destroy(&box->cond);
	if (res) {
		ULOGE(
			"BUG: dynmbox destroyed while in use by a producer"
		);
	}
	pthread_mutex_destroy(&box->lock);
	destroy_notify_channel(box);
	free(box->bufmem);
	free(box);
}

int dynmbox_get_read_fd(const struct dynmbox *box)
{
#ifdef _WIN32
	if (!box)
		return -1;
	/* Winsock2's socket() returns the unsigned type SOCKET,
	 * which is a 32-bit type for WIN32 and a 64-bit type for WIN64;
	 * as we cast the result to an int, return an error if the
	 * returned value does not fit into 31 bits. */
	if (box->rfd > (SOCKET)0x7fffffff) {
		ULOGE("Cannot return socket as a 32-bit 'int'");
		return -1;
	}
	return (int)box->rfd;
#else
	return box ? box->fds[0] : -1;
#endif
}

ssize_t dynmbox_get_max_size(const struct dynmbox *box)
{
	return box ? (ssize_t) box->max_msg_size : -EINVAL;
}

static unsigned int increment(size_t *idx, size_t amount)
{
	size_t oldidx = *idx;
	size_t following = (ALLOCATED_LEN - oldidx);
	assert(amount <= ALLOCATED_LEN);
	if (amount >= following)
		*idx = amount - following;
	else
		*idx += amount;
	return oldidx;
}

static inline bool rbuf_is_empty(const struct dynmbox *box)
{
	return box->used == 0;
}

static inline bool rbuf_is_full(const struct dynmbox *box)
{
	return box->used == ALLOCATED_LEN;
}

static inline size_t rbuf_space_left(const struct dynmbox *box)
{
	return ALLOCATED_LEN - box->used;
}

static inline size_t rbuf_space_used(const struct dynmbox *box)
{
	return box->used;
}

/* Copy data into the ring buffer, updating the write cursor and used space.
 * It is the caller's resposibility to check that there is enough space in the
 * ring buffer.
 */
static void rbuf_write(struct dynmbox *box, const void *data, size_t len)
{
	const uint8_t *bytes = data;
	size_t done = 0;
	size_t left = len;
	assert(len <= rbuf_space_left(box));

	/* Split the copy in chunks (at most two) */
	while (left > 0) {
		size_t idx = box->write_idx;
		size_t chunklen = ALLOCATED_LEN - box->write_idx;
		if (chunklen > left)
			chunklen = left;
		memcpy(&box->bufmem[idx], &bytes[done], chunklen);
		done += chunklen;
		left -= chunklen;
		increment(&idx, chunklen);
		box->write_idx = idx;
	}
	box->used += len;
}

/* Read data from the ring buffer, updating the read cursor and used space.
 * It is the caller's responsibility to ensure there is enough data in the ring
 * buffer, and that the output buffer's capacity is sufficient.
 */
static void rbuf_read(struct dynmbox *box, void *buf, size_t len)
{
	uint8_t *bytes = buf;
	size_t done = 0;
	size_t left = len;
	assert(len <= rbuf_space_used(box));

	/* Split the copy in chunks (at most two) */
	while (left > 0) {
		size_t idx = box->read_idx;
		size_t chunklen = ALLOCATED_LEN - box->read_idx;
		if (chunklen > left)
			chunklen = left;
		memcpy(&bytes[done], &box->bufmem[idx], chunklen);
		done += chunklen;
		left -= chunklen;
		increment(&idx, chunklen);
		box->read_idx = idx;
	}
	box->used -= len;
}

static int do_push(struct dynmbox *box, const void *msg, size_t msg_size)
{
	uint32_t hdr;

	/* Check remaining space */
	if (rbuf_space_left(box) < (msg_size + sizeof(hdr)))
		return -EAGAIN;

	hdr = (uint32_t)msg_size;

	/* Write header, then data */
	rbuf_write(box, &hdr, sizeof(hdr));
	rbuf_write(box, msg, msg_size);

	return 0;
}

static ssize_t do_peek(struct dynmbox *box, void *msg)
{
	uint32_t hdr;

	/* Check there is queued data */
	if (rbuf_is_empty(box))
		return -EAGAIN;

	assert(rbuf_space_used(box) >= (sizeof(hdr)));

	/* Write header, then data */
	rbuf_read(box, &hdr, sizeof(hdr));
	assert(hdr <= box->max_msg_size);
	assert(rbuf_space_used(box) >= hdr);
	rbuf_read(box, msg, hdr);

	return hdr;
}

int dynmbox_push(struct dynmbox *box,
			 const void *msg,
			 size_t msg_size)
{
	int res;

	if (!box || msg_size > box->max_msg_size || (msg_size > 0 && !msg))
		return -EINVAL;

	/* Lock */
	pthread_mutex_lock(&box->lock);

	/* Write message to ring buffer */
	res = do_push(box, msg, msg_size);
	if (res)
		goto fail;

	pthread_mutex_unlock(&box->lock);

	/* Write one byte into pipe/socket to signal mbox is readable */
	push_notify(box);

	return 0;
fail:
	pthread_mutex_unlock(&box->lock);
	return res;
}

static int wait_cond_timed(struct dynmbox *box,
		const struct timespec *deadline)
{
	int res;
	res = pthread_cond_timedwait(&box->cond, &box->lock, deadline);
	return -res;
}

static int wait_cond(struct dynmbox *box)
{
	int res;
	res = pthread_cond_wait(&box->cond, &box->lock);
	if (res != 0)
		return -res;
	return 0;
}

int dynmbox_push_block(struct dynmbox *box, const void *msg,
		size_t msg_size, unsigned int timeout_ms)
{
	int res;
	struct timeval tv_now;
	struct timespec ts_now;
	struct timespec ts_abs;

	if (!box || msg_size > box->max_msg_size || (msg_size > 0 && !msg))
		return -EINVAL;

	if (timeout_ms > 0) {
		res = gettimeofday(&tv_now, NULL);
		if (res) {
			res = -errno;
			ULOG_ERRNO("gettimeofday()", -res);
			return res;
		}
		time_timeval_to_timespec(&tv_now, &ts_now);
		time_timespec_add_us(&ts_now, timeout_ms * 1000, &ts_abs);
	}

	pthread_mutex_lock(&box->lock);

	/* Block until a buffer is available */
	while (rbuf_space_left(box) < (msg_size + sizeof(uint32_t))) {
		if (timeout_ms == 0)
			res = wait_cond(box);
		else
			res = wait_cond_timed(box, &ts_abs);
		if (res)
			goto fail;
	}

	/* Write message to ring buffer (should always succeed) */
	res = do_push(box, msg, msg_size);
	assert(res == 0);
	if (res)
		goto fail; /* Return error (NDEBUG case) */

	pthread_mutex_unlock(&box->lock);

	/* Signal consumers */
	push_notify(box);

	return 0;
fail:
	pthread_mutex_unlock(&box->lock);
	return res;
}

ssize_t dynmbox_peek(struct dynmbox *box, void *msg)
{
	int res;
	ssize_t msglen;

	if (!msg || !box)
		return -EINVAL;

	/* Lock */
	res = pthread_mutex_lock(&box->lock);
	if (res)
		return -res;

	/* Check not empty */
	if (rbuf_is_empty(box)) {
		msglen = -EAGAIN;
		goto fail;
	}

	/* Read one byte from pipe/socket, ignoring failure */
	pop_notify(box);

	/* Read message from ring buffer */
	msglen = do_peek(box, msg);
	if (msglen < 0)
		goto fail;

	/* Signal condition */
	pthread_cond_signal(&box->cond);

	/* Unlock */
	pthread_mutex_unlock(&box->lock);

	return msglen;
fail:
	/* Unlock */
	pthread_mutex_unlock(&box->lock);
	return msglen;
}
