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
#ifndef _FUTILS_SAFEW_H
#define _FUTILS_SAFEW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Safe file write structure (used in the same way as a regular FILE
 * pointer)
 */
struct futils_safew_file;

/**
 * @brief Safe write fopen
 *
 * @param pathname of file to open
 *
 * @return safe file write structure or NULL on error
 */
struct futils_safew_file *futils_safew_fopen(const char *pathname);

/**
 * @brief Safe write fclose without validating new file
 *
 * @param pointer to a safe file write structure
 *
 * @return 0 if success, -1 on error
 */
int futils_safew_fclose_rollback(struct futils_safew_file *safew_fp);

/**
 * @brief Safe write fclose validating new file
 *
 * @param pointer to a safe file write structure
 *
 * @return 0 if success, -1 on error
 */
int futils_safew_fclose_commit(struct futils_safew_file *safew_fp);

/**
 * @brief Safe write fprintf
 *
 * @param same as fprintf but with pointer to a safe file write structure
 * instead of a file pointer
 *
 * @return the number of characters printed (excluding the null byte
 *  used to end output to strings) if success, negative value on error
 */
__attribute__((format (printf, 2, 3)))
int futils_safew_fprintf(struct futils_safew_file *safew_fp,
			 const char *format, ...);

 /**
 * @brief Safe write fwrite
 *
 * @param same as fwrite but with pointer to a safe file write structure
 * instead of a file pointer
 *
 * @return the number of items transfered if success. If size is 1
 * this number is the number of bytes printed
 */
size_t futils_safew_fwrite(const void *ptr, size_t size, size_t nmemb,
			   struct futils_safew_file *safew_fp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FUTILS_SAFEW_H */
