/******************************************************************************
 * Copyright (c) 2020 Parrot S.A.
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
 * @file varint.h
 *
 * @brief variable-length quantity (VLQ) API
 *        https://en.wikipedia.org/wiki/Variable-length_quantity
 *
 *****************************************************************************/

/**
 * Reads a variable unsigned integer from data.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_read_u16(const uint8_t *src, size_t src_len,
		uint16_t *val, size_t *val_len);

/**
 * Writes a variable unsigned integer in data.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ; should be greater or equal to 3.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_write_u16(uint8_t *dst, size_t dst_len,
		uint16_t val, size_t *val_len);

/**
 * Reads a variable integer from data.
 *
 * Uses ZigZag encoding.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_read_i16(const uint8_t *src, size_t src_len,
		int16_t *val, size_t *val_len);

/**
 * Writes a variable integer in data.
 *
 * Uses ZigZag encoding.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ; should be greater or equal to 3.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_write_i16(uint8_t *dst, size_t dst_len,
		int16_t val, size_t *val_len);

/**
 * Reads a variable unsigned integer from data.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_read_u32(const uint8_t *src, size_t src_len,
		uint32_t *val, size_t *val_len);

/**
 * Writes a variable unsigned integer in data.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ; should be greater or equal to 5.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_write_u32(uint8_t *dst, size_t dst_len,
		uint32_t val, size_t *val_len);

/**
 * Reads a variable integer from data.
 *
 * Uses ZigZag encoding.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_read_i32(const uint8_t *src, size_t src_len,
		int32_t *val, size_t *val_len);

/**
 * Writes a variable integer in data.
 *
 * Uses ZigZag encoding.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ; should be greater or equal to 5.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_write_i32(uint8_t *dst, size_t dst_len,
		int32_t val, size_t *val_len);

/**
 * Reads a variable unsigned integer from data.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_read_u64(const uint8_t *src, size_t src_len,
		uint64_t *val, size_t *val_len);

/**
 * Writes a variable unsigned integer in data.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ; should be greater or equal to 10.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_write_u64(uint8_t *dst, size_t dst_len,
		uint64_t val, size_t *val_len);

/**
 * Reads a variable integer from data.
 *
 * Uses ZigZag encoding.
 *
 * @param src : Source where read.
 * @param src_len : Source length.
 * @param val[out] : Value read.
 * @param val_len[out] : Length in byte read from the source.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_read_i64(const uint8_t *src, size_t src_len,
		int64_t *val, size_t *val_len);

/**
 * Writes a variable integer in data.
 *
 * Uses ZigZag encoding.
 *
 * @param dst : Destination where write.
 * @param dst_len : Destination length ; should be greater or equal to 10.
 * @param val : Value to write.
 * @param val_len[out] : Length in byte written in the destination.
 *
 * @return 0 in case of success, negative errno value in case of error.
 */
int futils_varint_write_i64(uint8_t *dst, size_t dst_len,
		int64_t val, size_t *val_len);
