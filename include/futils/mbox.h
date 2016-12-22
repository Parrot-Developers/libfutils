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
 * @file mbox.h
 *
 * @brief Simple mailbox mechanism guaranteeing atomic read/write
 *
 *****************************************************************************/

#ifndef _MBOX_H_
#define _MBOX_H_

#ifdef __cplusplus
extern "C" {
#endif

struct mbox;

/**
 * @brief Create a mail box
 *
 * @param[in] msg_size The maximum size for a message
 *     Must be defined as follow : 0 < msg_size < PIPE_BUF
 *
 * @return Handle for future uses on success
 *         NULL on error
 */
struct mbox *mbox_new(size_t msg_size);

/**
 * @brief Destroy a mail box
 *
 * @param[in] mbox Handle of the mail box
 */
void mbox_destroy(struct mbox *box);

/**
 * @brief Get file descriptor for received data
 *
 * @param[in] mbox Handle of the mail box
 *
 * @return The file descriptor on success
 *         -1 on error
 */
int mbox_get_read_fd(const struct mbox *box);

/**
 * @brief Write a message in the mail box
 *
 * @param[in] mbox Handle of the mail box
 * @param[in] msg The message to send
 *
 * @return 0 on success,
 *         negative errno on error
 */
int mbox_push(struct mbox *box, const void *msg);

/**
 * @brief read a message from the mail box
 *
 * @param[in] mbox Handle of the mail box
 * @param[out] msg The message read
 *
 * @return 0 on success,
 *         negative errno on error
 */
int mbox_peek(struct mbox *box, void *msg);

#ifdef __cplusplus
}
#endif

#endif /* _MBOX_H_ */
