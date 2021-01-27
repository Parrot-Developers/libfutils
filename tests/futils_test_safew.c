/**
 * Copyright (c) 2021 Parrot S.A.
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
 * @file futils_test_safew.c
 *
 * @brief safew unit tests.
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <futils/safew.h>
#include "futils_test.h"
#include "../src/safew_types.h"

#define FILE_PATH "safew_test_file"
#define FILE_PATH_TMP "safew_test_file.tmp"
#define FILE_CONTENT "futils_safew_test_value"
#define PREVIOUS_FILE_CONTENT "XXXX"
#define CHECK_BUFFER_SIZE 64

static int test_safew_create_file(const char *file_path, char *value)
{
	size_t ret;
	FILE *fp;

	fp = fopen(file_path, "w");
	CU_ASSERT_PTR_NOT_NULL_FATAL(fp);

	ret = fwrite(value, 1, strlen(value), fp);
	if (ret != strlen(value))
		ret = -1;
	else
		ret = 0;

	fclose(fp);

	return ret;
}

static int test_safew_compare_file(const char *file_path, char *value)
{
	int ret;
	char buffer[CHECK_BUFFER_SIZE];
	FILE *fp;

	fp = fopen(file_path, "r");
	CU_ASSERT_PTR_NOT_NULL_FATAL(fp);

	ret = fread(buffer, 1, CHECK_BUFFER_SIZE, fp);
	if (ret != (int)strlen(value))
		ret = -1;
	else if (strncmp(buffer, value, strlen(value)) != 0)
		ret = -1;
	else
		ret = 0;

	fclose(fp);

	return ret;
}

static int test_safew_file_exists(const char *file_path)
{
	if (access(file_path, F_OK) == 0)
		return 0;

	return -1;
}

static void test_safew_create_fprintf(void)
{
	int ret;
	struct futils_safew_file *safew_fp;

	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fprintf(safew_fp, FILE_CONTENT);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), 0);

	CU_ASSERT_EQUAL(test_safew_compare_file(FILE_PATH, FILE_CONTENT), 0);
	CU_ASSERT_EQUAL(test_safew_file_exists(FILE_PATH_TMP), -1);

	unlink(FILE_PATH);
}

static void test_safew_create_fwrite(void)
{
	int ret;
	struct futils_safew_file *safew_fp;
	char *buffer = FILE_CONTENT;

	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fwrite(buffer, 1, strlen(buffer), safew_fp);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), 0);

	CU_ASSERT_EQUAL(test_safew_compare_file(FILE_PATH, FILE_CONTENT), 0);
	CU_ASSERT_EQUAL(test_safew_file_exists(FILE_PATH_TMP), -1);

	unlink(FILE_PATH);
}

static void test_safew_create_on_existing(void)
{
	int ret;
	struct futils_safew_file *safew_fp;

	/* First create file that must be overwritten */
	CU_ASSERT_EQUAL(test_safew_create_file(FILE_PATH,
					       PREVIOUS_FILE_CONTENT), 0);
	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fprintf(safew_fp, FILE_CONTENT);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), 0);

	CU_ASSERT_EQUAL(test_safew_compare_file(FILE_PATH, FILE_CONTENT), 0);
	CU_ASSERT_EQUAL(test_safew_file_exists(FILE_PATH_TMP), -1);

	unlink(FILE_PATH);
}

static void test_safew_create_interruption(void)
{
	int ret;
	struct futils_safew_file *safew_fp;

	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fprintf(safew_fp, FILE_CONTENT);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));

	/* simulate a shutdown, simple legacy close */
	CU_ASSERT_EQUAL(fclose(safew_fp->fp), 0);
	free(safew_fp);

	CU_ASSERT_EQUAL(test_safew_file_exists(FILE_PATH), -1);

	unlink(FILE_PATH_TMP);
	unlink(FILE_PATH);
}

static void test_safew_create_interruption_on_existing(void)
{
	int ret;
	struct futils_safew_file *safew_fp;

	/* First create file that must be overwritten */
	CU_ASSERT_EQUAL(test_safew_create_file(FILE_PATH,
					       PREVIOUS_FILE_CONTENT), 0);

	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fprintf(safew_fp, FILE_CONTENT);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));

	/* simulate a shutdown */
	CU_ASSERT_EQUAL(fclose(safew_fp->fp), 0);
	free(safew_fp);

	CU_ASSERT_EQUAL(test_safew_compare_file(FILE_PATH,
						PREVIOUS_FILE_CONTENT), 0);
	unlink(FILE_PATH_TMP);
	unlink(FILE_PATH);
}

static void test_safew_create_fail(void)
{
	int ret;
	struct futils_safew_file *safew_fp;

	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fprintf(safew_fp, FILE_CONTENT);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));

	safew_fp->failure = 1;
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), 0);

	CU_ASSERT_EQUAL(test_safew_file_exists(FILE_PATH), -1);

	unlink(FILE_PATH_TMP);
	unlink(FILE_PATH);
}

static void test_safew_create_fail_on_existing(void)
{
	int ret;
	struct futils_safew_file *safew_fp;

	/* First create file that must be overwriten */
	CU_ASSERT_EQUAL(test_safew_create_file(FILE_PATH,
					       PREVIOUS_FILE_CONTENT), 0);
	safew_fp = futils_safew_fopen(FILE_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(safew_fp);
	ret = futils_safew_fprintf(safew_fp, FILE_CONTENT);
	CU_ASSERT_EQUAL(ret, strlen(FILE_CONTENT));

	safew_fp->failure = 1;
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), 0);

	CU_ASSERT_EQUAL(test_safew_compare_file(FILE_PATH,
						PREVIOUS_FILE_CONTENT), 0);
	unlink(FILE_PATH_TMP);
	unlink(FILE_PATH);
}

CU_TestInfo s_safew_tests[] = {
	{(char *)"create_fprintf", &test_safew_create_fprintf},
	{(char *)"create_fwrite", &test_safew_create_fwrite},
	{(char *)"create_on_existing", &test_safew_create_on_existing},
	{(char *)"create_interrupt", &test_safew_create_interruption},
	{(char *)"create_interrupt_on_existing",
		 &test_safew_create_interruption_on_existing},
	{(char *)"create_fail", &test_safew_create_fail},
	{(char *)"create_fail_on_existing",
		 &test_safew_create_fail_on_existing},
	CU_TEST_INFO_NULL,
};
