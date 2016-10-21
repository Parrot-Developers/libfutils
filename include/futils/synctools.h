/******************************************************************************
 * Copyright (c) 2016 Parrot S.A.
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
 * @file synctools.h
 *
 * @brief Synchronize files and folders on file system
 *
 ******************************************************************************/

#ifndef _SYNCTOOLS_H_
#define _SYNCTOOLS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Synchronize a file
 *
 * @param[in] path The targeted file
 *
 * @return 0 on success
 *         negative errno on error
 */
int sync_file(const char *filepath);

/**
 * @brief Synchronize a folder
 *
 * @param[in] path The targeted folder
 *
 * @return 0 on success
 *         negative errno on error
 */
int sync_folder(const char *folderpath);

/**
 * @brief Synchronize a file and it's parent folder
 *
 * @param[in] path The targeted file
 *
 * @return 0 on success
 *         negative errno on error
 */
int sync_file_and_folder(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* _SYNCTOOLS_H_ */
