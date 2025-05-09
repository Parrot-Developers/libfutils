/******************************************************************************
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
 * @file safew.h
 *
 * @brief safe write file functions
 *
 *****************************************************************************/
#ifndef _FUTILS_SAFEW_TYPES_H
#define _FUTILS_SAFEW_TYPES_H

#include <stdio.h>

#define SAFEW_TMP_SUFFIX ".tmp"
#define SAFEW_CRC_SUFFIX ".crc"
/* exclude final '\0' from char sizeof */
#define SAFEW_TMP_SUFFIX_SIZE (sizeof(SAFEW_TMP_SUFFIX) - 1)
#define SAFEW_CRC_SUFFIX_SIZE (sizeof(SAFEW_CRC_SUFFIX) - 1)
#define SAFEW_CRC_TMP_SUFFIX_SIZE (sizeof(SAFEW_TMP_SUFFIX) + \
				   sizeof(SAFEW_CRC_SUFFIX) - 2)

#define SAFEW_PATH_MAX_LEN 128
#ifdef THREADX_OS
#define SAFEW_BUFFER_SIZE 128
#endif

struct futils_safew_file {
	FILE *fp;
	char path[SAFEW_PATH_MAX_LEN];
	char tmp_path[SAFEW_PATH_MAX_LEN + SAFEW_TMP_SUFFIX_SIZE];
	int failure;
};

struct futils_safew_crc {
	char tmp_path[SAFEW_PATH_MAX_LEN + SAFEW_CRC_TMP_SUFFIX_SIZE];
	char path[SAFEW_PATH_MAX_LEN + SAFEW_CRC_SUFFIX_SIZE];
};

#endif /* _FUTILS_SAFEW_TYPES_H */
