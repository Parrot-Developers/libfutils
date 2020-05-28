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

#ifndef __MINGW32__

/* This function is used to flush the dynmbox contents in tests where there's
 * only a producer */
static ssize_t flush_mbox(struct dynmbox *box)
{
	int ret;
	int nbytes = 0;
	char buf;

	if (!box)
		return -EINVAL;

	do {
		ret = read(dynmbox_get_read_fd(box), &buf, 1);
		nbytes += ret >= 0 ? 1 : 0;
	} while (ret > 0);

	return ret == -EAGAIN ? nbytes : ret;
}

static void test_dynmbox_creation(void)
{
	struct dynmbox *box1, *box2, *box3;

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

	/* Push a message of almost the maximum size, leaving just enough room
	 * in the pipe buffer for the header + one byte */
	ret = dynmbox_push(box2, very_large_msg, very_large_msg_max_size - 5);
	CU_ASSERT_EQUAL(ret, 0);
	/* The buffer has not been read yet, and we try to push a message of
	 * only 2 bytes, which exceeds the pipe buffer capacity */
	ret = dynmbox_push(box2, very_large_msg, 2);
	CU_ASSERT_EQUAL(ret, -EAGAIN);

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

	/* Create a box with a size of PIPE_BUF */
	box = dynmbox_new(PIPE_BUF);
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	/* send empty message and check that read size is 0 */
	error = send_and_receive_msg(box, NULL, msg_read, 0);
	CU_ASSERT_EQUAL(error, 0);

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
	CU_TEST_INFO_NULL,
};

#else

CU_TestInfo s_dynmbox_tests[] = {
	CU_TEST_INFO_NULL,
};

#endif
