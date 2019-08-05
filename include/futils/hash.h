/******************************************************************************
 * Copyright (c) 2015 Parrot S.A.
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
 * @file hash.h
 *
 * @brief hash
 *
 *****************************************************************************/

#ifndef _HASH_H_
#define _HASH_H_

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <futils/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FUTILS_LIST

/**
 * hash entry container
 */
struct hash_entry {
	struct list_node node;		/* node in hash list entries */
	int is_const;			/* is entry const */
	union {
		void *data;		/* entry data */
		const void *const_data;	/* entry const data */
	};
	uint32_t key;			/* entry key */
	struct hash_entry *next;	/* next entry with same hash value */
};

/**
 * hash structure
 */
struct hash {
	struct hash_entry **buckets;	/* hash table buckets */
	uint32_t size;			/* hash table size */
	struct list_node entries;	/* node entries */
};

/**
 * create a new hash table
 * @param hash
 * @param size
 * @return 0 on success
 */
int futils_hash_init(struct hash *hash, size_t size);

/**
 * destroy hash table
 * @param hash
 * @return 0 on success
 */
int futils_hash_destroy(struct hash *hash);

/**
 * insert an entry in hash table
 *
 * @param hash hash table
 * @param key entry key
 * @param data entry data to be stored
 * @return 0 if entry inserted, -EEXIST if another entry with same key.
 */
int futils_hash_insert(struct hash *hash, uint32_t key, void *data);

/**
 * insert a const entry in hash table
 *
 * @param hash hash table
 * @param key entry key
 * @param data entry data to be stored
 * @return 0 if entry inserted, -EEXIST if another entry with same key.
 */
int futils_hash_insert_const(struct hash *hash, uint32_t key,
			     const void *data);

/**
 * remove an entry from hash table
 *
 * @param tab hash table
 * @param key entry key to be removed
 * @return 0 if entry is found and removed
 */
int futils_hash_remove(struct hash *hash, uint32_t key);

/**
 * remove all entries from hash table
 *
 * @param tab hash table
 * @return 0 if hash is cleared
 */
int futils_hash_remove_all(struct hash *hash);

/**
 * lookup to an entry in hash table
 *
 * @param tab hash table
 * @param key entry key
 * @param data entry data pointer if entry found
 * @return return 0 if entry is found
 */
int futils_hash_lookup(const struct hash *hash,
		       uint32_t key, void **data);

/**
 * lookup to a const entry in hash table
 *
 * @param tab hash table
 * @param key entry key
 * @param data entry data pointer if entry found
 * @return return 0 if entry is found
 */
int futils_hash_lookup_const(const struct hash *hash, uint32_t key,
			     const void **data);


/* Define aliases for functions for compatibility with the previous API:
 * we need symbol names to be prefixed only, not the actual definition
 * in the header */

#define hash_init(_hash, _size) futils_hash_init((_hash), (_size))

#define hash_destroy(_hash) futils_hash_destroy((_hash))

#define hash_insert(_hash, _key, _data) \
		futils_hash_insert((_hash), (_key), (_data))

#define hash_insert_const(_hash, _key, _data) \
		futils_hash_insert_const((_hash), (_key), (_data))

#define hash_remove(_hash, _key) futils_hash_remove((_hash), (_key))

#define hash_remove_all(_hash) futils_hash_remove_all((_hash))

#define hash_lookup(_hash, _key, _data) \
		futils_hash_lookup((_hash), (_key), (_data))

#define hash_lookup_const(_hash, _key, _data) \
		futils_hash_lookup_const((_hash), (_key), (_data))


#endif /* FUTILS_LIST */

#ifdef __cplusplus
}
#endif

#endif /*_HASH_H_*/
