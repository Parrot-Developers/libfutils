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
 * @file inotify.h
 *
 * @brief inotify wrapper to ease its usage in various Parrot tools
 *
 *****************************************************************************/
#ifndef _FUTILS_INOTIFY_H
#define _FUTILS_INOTIFY_H

#include <stdarg.h>
#include <stdint.h>
#include <sys/inotify.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*----------------------------------------------------------------------------*/
/**
 * Inotify callback type, called from inotify_process_fd()
 */
typedef void (*inotify_cb_t)(struct inotify_event *event, void *data);

/**
 * Open an inotify file descriptor, add watch on given path with given mask
 *
 * Additional watches may be later added with inotify_add_watch().
 * The returned file descriptor should be polled for POLLIN events and
 * processed with inotify_process_fd().
 *
 * @param path:         Path to monitor with inotify
 * @param mask:         Inotify mask of events (IN_CREATE, etc)
 * @return              Inotify file descriptor, or -1 if an error occurred
 */
int inotify_create(const char *path, uint32_t mask);

/**
 * Release inotify file descriptor
 *
 * @param fd:           Inotify file descriptor
 */
void inotify_destroy(int fd);

/**
 * Iterate through received inotify events
 *
 * This function should be called when a POLLIN event has been detected on
 * an inotify file descriptor.
 *
 * @param fd:           Inotify file descriptor
 * @param cb:           Iterator callback
 * @param data:         Iterator callback user pointer
 */
void inotify_process_fd(int fd, inotify_cb_t cb, void *data);


/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FUTILS_INOTIFY_H */
