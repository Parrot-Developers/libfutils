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
#define FILE_PATH_BCK "safew_test_file.bck"
#define FILE_PATH_CRC "safew_test_file.crc"
#define FILE_PATH_CRC_TMP "safew_test_file.crc.tmp"
#define FILE_PATH_CRC_BCK "safew_test_file.crc.bck"
#define RANDOM_CRC "XXXX"
#define FILE_CONTENT "futils_safew_test_value"
#define FILE_CONTENT_MODIFIED "futils_safew_test_valuf"
#define PREVIOUS_FILE_CONTENT "XXXX"
#define CHECK_BUFFER_SIZE 64

static int test_safew_create_file(const char *file_path, char *value)
{
	size_t ret;
	FILE *fp;

	fp = fopen(file_path, "w");
	if (fp == NULL)
		return -1;

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
	if (fp == NULL)
		return -1;

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
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), -1);

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
	CU_ASSERT_EQUAL(futils_safew_fclose_commit(safew_fp), -1);

	CU_ASSERT_EQUAL(test_safew_compare_file(FILE_PATH,
						PREVIOUS_FILE_CONTENT), 0);
	unlink(FILE_PATH_TMP);
	unlink(FILE_PATH);
}

static void clean_fs()
{
	/* remove all the files created by the test */
	unlink(FILE_PATH);
	unlink(FILE_PATH_TMP);
	unlink(FILE_PATH_CRC);
	unlink(FILE_PATH_BCK);
	unlink(FILE_PATH_CRC_TMP);
	unlink(FILE_PATH_CRC_BCK);
}

static int assert_crc_check_ok()
{
	int ret;

	/* file check should return 0 */
	ret = futils_safew_file_check(FILE_PATH);
	if (ret < 0) {
		ULOGE("a payload/crc pair should be valid");
		return ret;
	}

	/* payload and crc files should be present on the file system */
	ret = test_safew_file_exists(FILE_PATH);
	if (ret < 0) {
		ULOGE("file %s should exist", FILE_PATH);
		return ret;
	}

	ret = test_safew_file_exists(FILE_PATH_CRC);
	if (ret < 0) {
		ULOGE("file %s should exist", FILE_PATH_CRC);
		return ret;
	}

	/* temp files should not be found on the file system */
	ret = test_safew_file_exists(FILE_PATH_TMP);
	if (ret == 0) {
		ULOGE("file %s should not exist", FILE_PATH_TMP);
		return -EINVAL;
	}

	ret = test_safew_file_exists(FILE_PATH_CRC_TMP);
	if (ret == 0) {
		ULOGE("file %s should not exist", FILE_PATH_CRC_TMP);
		return -EINVAL;
	}

	/* check again to make sure the good files were renamed */
	ret = futils_safew_file_check(FILE_PATH);
	if (ret < 0) {
		ULOGE("a payload/crc pair should be valid");
		return ret;
	}

	/* make sure payload file content is the original */
	ret = test_safew_compare_file(FILE_PATH, FILE_CONTENT);
	if (ret < 0) {
		ULOGE("invalid payload content");
		return ret;
	}

	clean_fs();

	return 0;
}

static int assert_crc_check_ko()
{
	int ret = 0;

	/* file check should return an error */
	if (futils_safew_file_check(FILE_PATH) == 0) {
		ULOGE("no pair payload/crc should be valid");
		ret = -EINVAL;
	}

	/* none of the payload, crc, tmp payload or tmp crc should exist on the
	 * file system */
	if (test_safew_file_exists(FILE_PATH) == 0) {
		ULOGE("file %s should not exist", FILE_PATH);
		ret = -EINVAL;
	}

	if (test_safew_file_exists(FILE_PATH_CRC) == 0) {
		ULOGE("file %s should not exist", FILE_PATH_CRC);
		ret = -EINVAL;
	}

	if (test_safew_file_exists(FILE_PATH_TMP) == 0) {
		ULOGE("file %s should not exist", FILE_PATH_TMP);
		ret = -EINVAL;
	}

	if (test_safew_file_exists(FILE_PATH_CRC_TMP) == 0) {
		ULOGE("file %s should not exist", FILE_PATH_CRC_TMP);
		ret = -EINVAL;
	}

	clean_fs();

	return ret;
}

static int create_payload_crc_pair(const char* content)
{
	struct futils_safew_file *safew_fp;

	safew_fp = futils_safew_fopen(FILE_PATH);
	if (safew_fp == NULL)
		return -EPERM;

	futils_safew_fwrite(content, 1, strlen(content), safew_fp);

	return futils_safew_fclose_commit_with_crc(safew_fp);
}

#define ASSERT_OK(e) CU_ASSERT_EQUAL(e, 0)

static void test_safew_crc_check(void)
{
	/* check (none) */
	clean_fs();
	ASSERT_OK(assert_crc_check_ko());

	/* check payload only */
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT));
	ASSERT_OK(assert_crc_check_ko());

	/* check tmp payload */
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + tmp payload */
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + crc success */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + crc fail */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check crc only */
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ko());

	/* check tmp crc only */
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ko());

	/* check tmp crc + crc */
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ko());

	/* check tmp payload + crc (always ko even if it matches) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_TMP));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + tmp payload + crc success (payload valid) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp payload + crc failure
	 * (tmp payload valid but not tested) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + tmp crc success */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp crc failure */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check tmp payload + tmp crc success */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_TMP));
	ASSERT_OK(assert_crc_check_ok());

	/* check tmp payload + tmp crc failure */
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + tmp crc + crc success (crc) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp crc + crc success (tmp crc) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp crc + crc failure */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + tmp payload + tmp crc success (tmp payload valid) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp payload + tmp crc failure */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check tmp payload + crc + tmp crc success (crc valid) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ok());

	/* check tmp payload + crc + tmp crc failure */
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC, RANDOM_CRC));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ko());

	/* check payload + tmp payload + crc + tmp crc (payload & crc valid) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT_MODIFIED));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp payload + crc + tmp crc
	 * (tmp payload & tmp crc valid) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_TMP));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_TMP));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp payload + crc + tmp crc
	 * (all valid, tmp selected) */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	/* save files with a different name (not tmp or it would be removed) */
	ASSERT_OK(rename(FILE_PATH, FILE_PATH_BCK));
	ASSERT_OK(rename(FILE_PATH_CRC, FILE_PATH_CRC_BCK));
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT_MODIFIED));
	ASSERT_OK(rename(FILE_PATH_BCK, FILE_PATH_TMP));
	ASSERT_OK(rename(FILE_PATH_CRC_BCK, FILE_PATH_CRC_TMP));
	ASSERT_OK(assert_crc_check_ok());

	/* check payload + tmp payload + crc + tmp crc failure */
	ASSERT_OK(create_payload_crc_pair(FILE_CONTENT));
	ASSERT_OK(test_safew_create_file(FILE_PATH_TMP, FILE_CONTENT_MODIFIED));
	ASSERT_OK(test_safew_create_file(FILE_PATH, FILE_CONTENT_MODIFIED));
	ASSERT_OK(test_safew_create_file(FILE_PATH_CRC_TMP, RANDOM_CRC));
	ASSERT_OK(assert_crc_check_ko());
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
	{(char *)"crc_check", &test_safew_crc_check},
	CU_TEST_INFO_NULL,
};
