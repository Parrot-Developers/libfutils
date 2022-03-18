/******************************************************************************
 * Copyright (c) 2022 Parrot S.A.
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
 * @file string.h
 *
 * @brief string utility functions
 *
 *****************************************************************************/

#ifndef _FUTILS_STRING_H
#define _FUTILS_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Invalid characters for exFAT file systems, including all control codes from
 * 0x00 to 0x1F, Quotation mark ("), Asterisk (*), Forward Slash (/), Colon (:),
 * Less-than sign (<), Greater-than sign (>), Question mark (?), Back slash (\)
 * and Vertical bar (|).
 *
 * The NULL character is not tested here as it is the termination marker, and
 * thus can not appear inside a C-string.
 *
 * Source:
 * https://docs.microsoft.com/en-us/windows/win32/fileio/exfat-specification#773-filename-field
 *  */
#define FUTILS_STRING_EXFAT_FORBIDDEN                                          \
	L"\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf"                       \
	L"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"    \
	L"\"*/:<>?\\|"


/**
 * @brief Sanitizes an UTF-8 string
 *
 * @param input String to sanitize.
 * @param output Output string where the sanitized version will be stored.
 * @param max_output_size Size available in the `output' string. If this
 * function succeeds, then `output' will always be NULL-terminated.
 * @param forbidden_chars Optional pointer to a wide character string containing
 * characters that are forbidden in the input. Each character matching this
 * will be replaced by the value of `replacement_char'. If NULL, no check is
 * performed.
 * @param replacement_char if `forbidden_chars' is non-NULL, every matching
 * character will be replaced by this value in the `output' string.
 *
 * @return The number of bytes written to `output' (including the terminating
 * NULL byte) on success, or a negative errno value on error.
 */
int futils_string_sanitize_utf8(const char *input,
				char *output,
				size_t max_output_size,
				const wchar_t *forbidden_chars,
				wchar_t replacement_char);


/**
 * @brief Checks if a UTF-8 string is sane
 *
 * @param input String to check.
 * @param max_bytes Maximum number of bytes allowed for this string (including
 * terminating NULL byte). Not checked if set to 0.
 * @param max_wchars Maximum number of wide characters in the string (not
 * including the terminating NULL). Not checked if set to 0.
 * @param forbidden_chars Pointer to a wide character string containing
 * characters which must not appear in `input'
 *
 * @return 0 if the string is sane, -EPROTO/-E2BIG if it is not, or another
 * negative errno on error.
 */
int futils_string_check_utf8(const char *input,
			     size_t max_bytes,
			     size_t max_wchars,
			     const wchar_t *forbidden_chars);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FUTILS_STRING_H */
