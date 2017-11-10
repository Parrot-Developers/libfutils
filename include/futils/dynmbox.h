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
 * @file dynmbox.h
 *
 * @brief One-to-one non-blocking mailbox mechanism for messages of varying size
 *
 * @details This mechanism allows a single producer thread to send messages
 * to a single consumer thread. The messages' size can vary within the capacity
 * declared at the mailbox's initialization. Write & read are non-blocking, and
 * the messages will be read in the order in which they are sent (even if their
 * size exceeds PIPE_BUF).
 * The behaviour in the case of multiple producers is completely unspecified,
 * though no runtime check is done in order to verify that there is only one
 * producer per mailbox.
 *
 *****************************************************************************/

#ifndef __DYNMBOX_H__
#define __DYNMBOX_H__

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum size of a dynmbox message : this is based on the default pipe
 * capacity of 65536 bytes, which can be modified using the fcntl F_SETPIPE_SZ
 * operation. See pipe(7) for details */
#define DYNMBOX_MAX_SIZE (65536 - sizeof(size_t))

struct dynmbox;

/**
 * @brief Create a mail box
 *
 * @param[in] max_msg_size The maximum size of a message
 *
 * @return Handle for future uses on success
 *         NULL on error
 */
struct dynmbox *dynmbox_new(size_t max_msg_size);

/**
 * @brief Destroy a mail box
 *
 * @param[in] box Handle of the mail box
 */
void dynmbox_destroy(struct dynmbox *box);

/**
 * @brief Get file descriptor for received data
 *
 * @param[in] box Handle of the mail box
 *
 * @return The file descriptor on success
 *         -1 on error
 */
int dynmbox_get_read_fd(const struct dynmbox *box);

/**
 * @brief Get maximum size of the messages of this mail box
 *
 * @param[in] box Handle of the mail box
 *
 * @return size on success,
 *         -EINVAL on error
 */
ssize_t dynmbox_get_max_size(const struct dynmbox *box);

/**
 * @brief Write a message in the mail box
 *
 * @param[in] box Handle of the mail box
 * @param[in] msg The message to send
 * @param[in] msg_size Size of the message to send
 *
 * @return 0 if all data was written
 *         -EINVAL in case of invalid arguments,
 *         -EAGAIN if the message box is already full, or if adding the message
 * would overflow the pipe capacity, or if the write could not be completed
 */
int dynmbox_push(struct dynmbox *box,
		     const void *msg,
		     size_t msg_size);

/**
 * @brief read a message from the mail box
 *
 * @param[in] box Handle of the mail box
 * @param[out] msg The message read
 *
 * @return size of what has been read on success,
 *         -EINVAL in case of invalid arguments,
 *         -EPIPE if the pipe closed,
 *         other negative errno on error in read
 */
ssize_t dynmbox_peek(struct dynmbox *box, void *msg);

#ifdef __cplusplus
}
#endif

#endif /* __DYNMBOX_H__ */
