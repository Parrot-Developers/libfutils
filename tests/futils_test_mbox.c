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

#ifdef _WIN32
#  include <winsock2.h>
#endif /* _WIN32 */

struct message {
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
};

static const struct message s_msg1 = {
	.u16 = 16, .u32 = 32, .u64 = 64
};

static const struct message s_msg2 = {
	.u16 = 128, .u32 = 256, .u64 = 512
};

static void test_mbox(void)
{
	struct mbox *box = NULL;
	int ret = 0;
	int fd = 0;
	struct message out;
	fd_set rfds;
	struct timeval timeout;

#ifdef _WIN32
	/* Initialize winsock API */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 0), &wsadata);
#endif /* _WIN32 */

	/* Invalid size */
	box = mbox_new(0);
	CU_ASSERT_PTR_NULL(box);

	/* Create box */
	box = mbox_new(sizeof(struct message));
	CU_ASSERT_PTR_NOT_NULL_FATAL(box);

	/* Get fd */
	fd = mbox_get_read_fd(NULL);
	CU_ASSERT_EQUAL(fd, -1);
	fd = mbox_get_read_fd(box);
	CU_ASSERT(fd >= 0);

	/* Invalid push */
	ret = mbox_push(NULL, &s_msg1);
	CU_ASSERT_EQUAL(ret, -EINVAL);
	ret = mbox_push(box, NULL);
	CU_ASSERT_EQUAL(ret, -EINVAL);

	/* Push msg1 */
	ret = mbox_push(box, &s_msg1);
	CU_ASSERT_EQUAL(ret, 0);

	/* Push msg2 */
	ret = mbox_push(box, &s_msg2);
	CU_ASSERT_EQUAL(ret, 0);

	/* Invalid peek */
	ret = mbox_peek(NULL, &out);
	CU_ASSERT_EQUAL(ret, -EINVAL);
	ret = mbox_peek(box, NULL);
	CU_ASSERT_EQUAL(ret, -EINVAL);

	/* Peek msg1 */
	memset(&out, 0, sizeof(out));
	ret = mbox_peek(box, &out);
	CU_ASSERT_EQUAL(ret, 0);
	ret = memcmp(&out, &s_msg1, sizeof(struct message));
	CU_ASSERT_EQUAL(ret, 0);

	/* Peek msg2 */
	memset(&out, 0, sizeof(out));
	ret = mbox_peek(box, &out);
	CU_ASSERT_EQUAL(ret, 0);
	ret = memcmp(&out, &s_msg2, sizeof(struct message));
	CU_ASSERT_EQUAL(ret, 0);

	/* No more messages */
	memset(&out, 0, sizeof(out));
	ret = mbox_peek(box, &out);
	CU_ASSERT_EQUAL(ret, -EAGAIN);

	/* Wait read fd, timeout expected */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	ret = select(fd + 1, &rfds, NULL, NULL, &timeout);
	CU_ASSERT_EQUAL(ret, 0);

	ret = mbox_push(box, &s_msg1);
	CU_ASSERT_EQUAL(ret, 0);

	/* Wait read fd, ready expected */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	ret = select(fd + 1, &rfds, NULL, NULL, &timeout);
	CU_ASSERT_EQUAL(ret, 1);

	mbox_destroy(NULL);
	mbox_destroy(box);

#ifdef _WIN32
	/* Cleanup winsock API */
	WSACleanup();
#endif /* _WIN32 */
}

CU_TestInfo s_mbox_tests[] = {
	{(char *)"mbox", &test_mbox},
};
