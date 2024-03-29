/**
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
 * @file futils_test_dynmbox.c
 *
 * @brief dynmbox unit tests
 *
 */

#include "futils_test.h"

#include <pthread.h>
#include <signal.h>

#ifdef _WIN32
# include <winsock2.h>
# define PIPE_BUF 4096
#endif

#define CONCURRENT_MSGLEN (2 * PIPE_BUF)
#define CONCURRENT_ITERATIONS 100
static sig_atomic_t concurrent_thread_exit;
static uint8_t flush_mbox_buf[DYNMBOX_MAX_SIZE];

#ifdef _WIN32
static inline void init_winsock(void)
{
	/* Initialize winsock API */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 0), &wsadata);
}
#else
static inline void init_winsock(void) {}
#endif

/* This function is used to flush the dynmbox contents in tests where there's
 * only a producer */
static ssize_t flush_mbox(struct dynmbox *box)
{
	ssize_t nbytes;
	ssize_t total = 0;

	if (!box)
		return -EINVAL;

	do {
		nbytes = dynmbox_peek(box, flush_mbox_buf);
		total += nbytes >= 0 ? nbytes : 0;
	} while (nbytes >= 0);

	return nbytes == -EAGAIN ? total : nbytes;
}

/* This function is used to fill the mbox to max capacity */
static int fill_mbox(struct dynmbox *box, const void *msg, size_t msglen)
{
	int ret;

	if (!box)
		return -EINVAL;

	do {
		ret = dynmbox_push(box, msg, msglen);
	} while (!ret);

	return (ret == -EAGAIN) ? 0 : ret;
}

static void *test_dynmbox_concurrent_thread(void *arg)
{
	static uint8_t msg[CONCURRENT_MSGLEN];
	struct dynmbox *mbox = arg;
	int res;
	unsigned int i;

	memset(msg, 0x55, sizeof(msg));

	for (i = 0; i < CONCURRENT_ITERATIONS; i++) {
		do {
			if (concurrent_thread_exit)
				break;
			res = dynmbox_push(mbox, &msg, sizeof(msg));
		} while (res == -EAGAIN);
	}
	return NULL;
}

static void test_dynmbox_concurrent(void)
{
	static uint8_t msg[CONCURRENT_MSGLEN];
	struct dynmbox *box;
	pthread_t tid;
	int res;
	int fd = 0;
	fd_set rfds;
	struct timeval timeout;
	unsigned int i;

	init_winsock();

	box = dynmbox_new(CONCURRENT_MSGLEN);
	CU_ASSERT_PTR_NOT_NULL(box);

	concurrent_thread_exit = 0;
	res = pthread_create(&tid, NULL, test_dynmbox_concurrent_thread,
			(void *)box);
	CU_ASSERT_EQUAL(res, 0);

	/* Get fd */
	fd = dynmbox_get_read_fd(box);
	CU_ASSERT(fd >= 0);

	for (i = 0; i < CONCURRENT_ITERATIONS; i++) {
		int early_exit = 0;

		/* Wait for message */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		res = select(fd + 1, &rfds, NULL, NULL, &timeout);
		if (res <= 0) {
			CU_FAIL("select() failed or timed out (msg lost?)");
			break;
		}

		do {
			res = dynmbox_peek(box, &msg);
			if (res == -EAGAIN) {
				CU_FAIL(
					"select() passed, but we still got -EAGAIN"
				);
				early_exit = 1;
			}
			if ((res < 0) && (res != -EAGAIN)) {
				CU_FAIL("dynmbox_peek(): unexpected error");
				early_exit = 1;
			}
			if ((res >= 0) && (res != CONCURRENT_MSGLEN)) {
				CU_FAIL("Unexpected message length");
				early_exit = 1;
			}
		} while (res == -EAGAIN);
		if (early_exit)
			break;
	}

	concurrent_thread_exit = 1;
	pthread_join(tid, NULL);
	dynmbox_destroy(box);
}

static void test_dynmbox_creation(void)
{
	struct dynmbox *box1, *box2, *box3;

	init_winsock();

	/* Message size smaller than DYNMBOX_MAX_SIZE => the box should be
	 * created successfully */
	box1 = dynmbox_new(10);
	CU_ASSERT_PTR_NOT_NULL(box1);

	/* Message size just equal to DYNMBOX_MAX_SIZE => this should work */
	box2 = dynmbox_new(DYNMBOX_MAX_SIZE);
	CU_ASSERT_PTR_NOT_NULL(box2);

	/* Message size bigger than DYNMBOX_MAX_SIZE => the box creation should
	 * fail */
	box3 = dynmbox_new(DYNMBOX_MAX_SIZE + 1);
	CU_ASSERT_PTR_NULL(box3);

	dynmbox_destroy(box1);
	dynmbox_destroy(box2);
}

static void test_dynmbox_get_read_fd(void)
{
	struct dynmbox *box;
	int ret;

	init_winsock();

	/* Create a box */
	box = dynmbox_new(10);
	CU_ASSERT_PTR_NOT_NULL(box);

	/* Ask for its read file descriptor : this should work */
	ret = dynmbox_get_read_fd(box);
	CU_ASSERT_TRUE(ret >= 0);

	/* Ask for the read file descriptor of an invalid box : this should
	 * fail */
	ret = dynmbox_get_read_fd(NULL);
	CU_ASSERT_TRUE(ret < 0);

	dynmbox_destroy(box);
}

static void test_dynmbox_get_max_size(void)
{
	struct dynmbox *box1, *box2, *box3, *box4;
	int ret;

	init_winsock();

	/* Getting max size of null dynmbox should fail */
	ret = dynmbox_get_max_size(NULL);
	CU_ASSERT_EQUAL(ret, -EINVAL);

	/* Create a box */
	box1 = dynmbox_new(10);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box1);

	/* Ask for its max size : this should work */
	ret = dynmbox_get_max_size(box1);
	CU_ASSERT_EQUAL(ret, 10);

	/* Create a box */
	box2 = dynmbox_new(PIPE_BUF - 1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box2);

	/* Ask for its max size : this should work */
	ret = dynmbox_get_max_size(box2);
	CU_ASSERT_EQUAL(ret, PIPE_BUF - 1);

	/* Create a box */
	box3 = dynmbox_new(10 * PIPE_BUF);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box3);

	/* Ask for its max size : this should work */
	ret = dynmbox_get_max_size(box3);
	CU_ASSERT_EQUAL(ret, 10 * PIPE_BUF);

	/* Create a box */
	box4 = dynmbox_new(DYNMBOX_MAX_SIZE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box4);

	/* Ask for its max size : this should work */
	ret = dynmbox_get_max_size(box4);
	CU_ASSERT_EQUAL(ret, DYNMBOX_MAX_SIZE);

	dynmbox_destroy(box1);
	dynmbox_destroy(box2);
	dynmbox_destroy(box3);
	dynmbox_destroy(box4);
}

static void test_dynmbox_push_smaller_than_pipe_buf(void)
{
	struct dynmbox *box;
	int ret;
	size_t i;
	int msg[10];
	size_t max_msg_size = sizeof(msg);

	init_winsock();

	/* initialize buffer to push */
	for (i = 0; i < SIZEOF_ARRAY(msg); i++)
		msg[i] = i;

	/* Create a box with a size smaller than PIPE_BUF */
	CU_ASSERT_TRUE_FATAL(max_msg_size < PIPE_BUF);
	box = dynmbox_new(max_msg_size);
	CU_ASSERT_PTR_NOT_NULL(box);

	/* Push a message of the max possible size : this should work */
	ret = dynmbox_push(box, msg, max_msg_size);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box);

	/* Push a message of a smaller size */
	ret = dynmbox_push(box, msg, max_msg_size / 2);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box);

	/* Push a message of a larger size */
	ret = dynmbox_push(box, msg, max_msg_size * 2);
	CU_ASSERT_EQUAL(ret, -EINVAL);

	dynmbox_destroy(box);
}

static void test_dynmbox_push_larger_than_pipe_buf(void)
{
	struct dynmbox *box1, *box2;
	int ret;
	size_t i;
	char large_msg[PIPE_BUF + 1], very_large_msg[DYNMBOX_MAX_SIZE];
	size_t large_msg_max_size = sizeof(large_msg);
	size_t very_large_msg_max_size = sizeof(very_large_msg);

	init_winsock();

	/* initialize buffers to push */
	for (i = 0; i < SIZEOF_ARRAY(large_msg); i++)
		large_msg[i] = i;

	for (i = 0; i < SIZEOF_ARRAY(very_large_msg); i++)
		very_large_msg[i] = i;

	/* Create a box with a size larger than PIPE_BUF */
	CU_ASSERT_TRUE_FATAL(large_msg_max_size > PIPE_BUF);
	box1 = dynmbox_new(large_msg_max_size);
	CU_ASSERT_PTR_NOT_NULL(box1);

	/* Push a message of the max possible size : this should work */
	ret = dynmbox_push(box1, large_msg, large_msg_max_size);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box1);

	/* Push a message of a smaller size */
	ret = dynmbox_push(box1, large_msg, large_msg_max_size / 2);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box1);

	/* Push a message of a larger size */
	ret = dynmbox_push(box1, large_msg, large_msg_max_size * 2);
	CU_ASSERT_EQUAL(ret, -EINVAL);

	/* Create a box with a size way larger than PIPE_BUF */
	CU_ASSERT_TRUE_FATAL(large_msg_max_size > PIPE_BUF);
	box2 = dynmbox_new(very_large_msg_max_size);
	CU_ASSERT_PTR_NOT_NULL(box2);

	/* Push a message of the max possible size : this should work */
	ret = dynmbox_push(box2, very_large_msg, very_large_msg_max_size);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box2);

	/* This should work twice ! */
	ret = dynmbox_push(box2, very_large_msg, very_large_msg_max_size);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box2);

	/* Push a message of a smaller size */
	ret = dynmbox_push(box2, very_large_msg, very_large_msg_max_size / 2);
	CU_ASSERT_EQUAL(ret, 0);

	flush_mbox(box2);

	/* Push a message of a larger size */
	ret = dynmbox_push(box2, very_large_msg, very_large_msg_max_size * 2);
	CU_ASSERT_EQUAL(ret, -EINVAL);

	dynmbox_destroy(box1);
	dynmbox_destroy(box2);
}

static void test_dynmbox_peek_smaller_than_pipe_buf(void)
{
	struct dynmbox *box;
	int ret;
	int msg_sent[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	int msg_read[10];
	size_t max_msg_size = sizeof(msg_sent);

	init_winsock();

	/* Create a box with a size smaller than PIPE_BUF */
	CU_ASSERT_TRUE_FATAL(max_msg_size < PIPE_BUF);
	box = dynmbox_new(max_msg_size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	/* Push a message of the max possible size : this should work */
	ret = dynmbox_push(box, msg_sent, max_msg_size);
	CU_ASSERT_EQUAL(ret, 0);

	memset(msg_read, 0, sizeof(msg_read));
	ret = dynmbox_peek(box, msg_read);
	CU_ASSERT_EQUAL(ret, max_msg_size);
	CU_ASSERT_EQUAL(memcmp(msg_read, msg_sent, ret), 0);

	/* Push a message of a smaller size : this should work */
	ret = dynmbox_push(box, msg_sent, max_msg_size / 2);
	CU_ASSERT_EQUAL(ret, 0);

	memset(msg_read, 0, sizeof(msg_read));
	ret = dynmbox_peek(box, msg_read);
	CU_ASSERT_EQUAL(ret, max_msg_size / 2);
	CU_ASSERT_EQUAL(memcmp(msg_read, msg_sent, ret), 0);

	dynmbox_destroy(box);
}

static int send_and_receive_msg(struct dynmbox *box,
				const char *src,
				char *dest,
				size_t msg_size)
{
	int ret;

	init_winsock();

	ret = dynmbox_push(box, src, msg_size);
	CU_ASSERT_EQUAL_FATAL(ret, 0);

	memset(dest, 0, msg_size);
	ret = dynmbox_peek(box, dest);
	CU_ASSERT_EQUAL_FATAL(ret, msg_size);

	return memcmp(dest, src, ret);
}

static void test_dynmbox_peek_larger_than_pipe_buf(void)
{
	struct dynmbox *box;
	int error;
	unsigned int i;
	char msg_sent[2 * PIPE_BUF];
	char msg_read[2 * PIPE_BUF];
	size_t max_msg_size = sizeof(msg_sent);

	init_winsock();

	for (i = 0; i < SIZEOF_ARRAY(msg_sent); i++)
		msg_sent[i] = i;

	/* Create a box with a size smaller than PIPE_BUF */
	CU_ASSERT_TRUE_FATAL(max_msg_size > PIPE_BUF);
	box = dynmbox_new(max_msg_size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	error = send_and_receive_msg(box,
					 msg_sent,
					 msg_read,
					 10 * sizeof(msg_sent[0]));
	CU_ASSERT_EQUAL(error, 0);

	error = send_and_receive_msg(box, msg_sent, msg_read, PIPE_BUF - 1);
	CU_ASSERT_EQUAL(error, 0);

	error = send_and_receive_msg(box, msg_sent, msg_read, max_msg_size);
	CU_ASSERT_EQUAL(error, 0);

	dynmbox_destroy(box);
}

static void test_dynmbox_peek_maximum_size(void)
{
	struct dynmbox *box;
	int error;
	unsigned int i;
	char msg_sent[DYNMBOX_MAX_SIZE];
	char msg_read[DYNMBOX_MAX_SIZE];
	size_t max_msg_size = sizeof(msg_sent);

	init_winsock();

	for (i = 0; i < SIZEOF_ARRAY(msg_sent); i++)
		msg_sent[i] = i;

	box = dynmbox_new(max_msg_size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	error = send_and_receive_msg(box,
					 msg_sent,
					 msg_read,
					 10 * sizeof(msg_sent[0]));
	CU_ASSERT_EQUAL(error, 0);

	error = send_and_receive_msg(box, msg_sent, msg_read, PIPE_BUF - 1);
	CU_ASSERT_EQUAL(error, 0);

	error = send_and_receive_msg(box, msg_sent, msg_read, 2 * PIPE_BUF);
	CU_ASSERT_EQUAL(error, 0);

	error = send_and_receive_msg(box, msg_sent, msg_read, max_msg_size);
	CU_ASSERT_EQUAL(error, 0);

	dynmbox_destroy(box);
}

static void test_dynmbox_peek_empty_message(void)
{
	struct dynmbox *box;
	int error;
	char msg_read[2 * PIPE_BUF];

	init_winsock();

	/* Create a box with a size of PIPE_BUF */
	box = dynmbox_new(PIPE_BUF);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	/* send empty message and check that read size is 0 */
	error = send_and_receive_msg(box, NULL, msg_read, 0);
	CU_ASSERT_EQUAL(error, 0);

	dynmbox_destroy(box);
}

static void test_dynmbox_push_block(void)
{
	struct dynmbox *box;
	int error;
	const char msg1[7] = {
		'd', 'y', 'n', 'm', 'b', 'o', 'x'
	};

	init_winsock();

	/* Create a box with any size */
	box = dynmbox_new(8);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	/* Pushing the first message should succeed immediately */
	error = dynmbox_push_block(box, msg1, sizeof(msg1), 0);
	CU_ASSERT_EQUAL(error, 0);

	/* Fill the queue */
	error = fill_mbox(box, msg1, sizeof(msg1));
	CU_ASSERT_EQUAL(error, 0);

	/* Blocking push should time out */
	error = dynmbox_push_block(box, msg1, sizeof(msg1), 100);
	CU_ASSERT_EQUAL(error, -ETIMEDOUT);

	/* Empty mbox */
	flush_mbox(box);

	dynmbox_destroy(box);
}

CU_TestInfo s_dynmbox_tests[] = {
	{(char *)"dynmbox creation", &test_dynmbox_creation},
	{(char *)"dynmbox get read fd", &test_dynmbox_get_read_fd},
	{(char *)"dynmbox get max size", &test_dynmbox_get_max_size},
	{(char *)"dynmbox push smaller than PIPE_BUF",
		&test_dynmbox_push_smaller_than_pipe_buf},
	{(char *)"dynmbox push larger than PIPE_BUF",
		&test_dynmbox_push_larger_than_pipe_buf},
	{(char *)"dynmbox peek smaller than PIPE_BUF",
		&test_dynmbox_peek_smaller_than_pipe_buf},
	{(char *)"dynmbox peek larger than PIPE_BUF",
		&test_dynmbox_peek_larger_than_pipe_buf},
	{(char *)"dynmbox peek at maximum size",
		&test_dynmbox_peek_maximum_size},
	{(char *)"dynmbox peek empty message",
		&test_dynmbox_peek_empty_message},
	{(char *)"dynmbox concurrency",
		&test_dynmbox_concurrent},
	{(char *)"dynmbox push_block",
		&test_dynmbox_push_block},
	CU_TEST_INFO_NULL,
};
