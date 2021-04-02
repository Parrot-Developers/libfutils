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
 * @file hash.c
 *
 * @brief hash
 *
 *****************************************************************************/

#include <string.h>
#include "futils/hash.h"

/**
 * hash prime table
 */
static const uint32_t futils_hash_prime_tab[] = {
	1, 2, 3, 7, 13, 31, 61, 127, 251, 509, 1021,
	2039, 4093, 8191, 16381, 32749, 65521, 131071,
	262139, 524287, 1048573, 2097143, 4194301, 8388593,
	16777213, 33554393, 67108859, 134217689, 268435399,
	536870909, 1073741789, 2147483647
};

static const uint32_t futils_hash_prime_tab_length =
			sizeof(futils_hash_prime_tab) /
			sizeof(futils_hash_prime_tab[0]);

#define DJB2_HASH_START (5381)
#define MULT33(_x_) (((_x_) << 5)+(_x_))

/**
 * Converts a 32bit unsigned key to a hash value.
 * using Daniel Bernstein djb2 hash function
 *
 * @param key
 * @return hash value
 */
static uint32_t hash_32(uint32_t key)
{
	uint32_t hash;
	hash = MULT33(DJB2_HASH_START) + (key & 0xff);
	key >>= 8;
	hash = MULT33(hash) + (key & 0xff);
	key >>= 8;
	hash = MULT33(hash) + (key & 0xff);
	key >>= 8;
	hash = MULT33(hash) + (key & 0xff);
	return hash;
}

int futils_hash_init(struct hash *hash, size_t size)
{
	size_t i;
	int ret;

	if (!hash) {
		ret = -EINVAL;
		goto error;
	}

	/* reset hash memory */
	memset(hash, 0 , sizeof(*hash));

	/* get upper prime number for tab size */
	for (i = 0; (futils_hash_prime_tab[i] <= size) &&
		    ((i + 1) < futils_hash_prime_tab_length); i++)
		;
	hash->size = futils_hash_prime_tab[i];

	/* allocate buckets */
	hash->buckets = calloc(hash->size, sizeof(struct hash_entry *));
	if (!hash->buckets) {
		ret = -ENOMEM;
		goto error;
	}

	list_init(&hash->entries);
	return 0;

error:
	return ret;
}

int futils_hash_remove_all(struct hash *hash)
{
	size_t i;
	struct hash_entry *entry, *next;

	if (!hash)
		return -EINVAL;

	for (i = 0; i < hash->size; i++) {
		entry = hash->buckets[i];
		while (entry) {
			next = entry->next;
			list_del(&entry->node);
			free(entry);
			entry = next;
		}
		hash->buckets[i] = NULL;
	}

	return 0;
}

int futils_hash_destroy(struct hash *hash)
{
	if (!hash)
		return -EINVAL;

	futils_hash_remove_all(hash);
	free(hash->buckets);
	memset(hash, 0 , sizeof(*hash));
	return 0;
}

static int futils_hash_insert_entry(struct hash *hash, uint32_t key,
				    struct hash_entry *new_entry)
{
	uint32_t hash_val;
	struct hash_entry *entry;

	/* compute entry hash from key */
	hash_val = hash_32(key);
	hash_val = hash_val % hash->size;

	entry = hash->buckets[hash_val];

	/**
	 * compare hash entries key to find if another entry
	 * with same key has been already added */
	while (entry) {
		if (entry->key == key) {
			/* obus_warn("hash key %d already exist !", key); */
			return -EEXIST;
		}

		entry = entry->next;
	}

	/* insert at list head */
	new_entry->next = hash->buckets[hash_val];
	hash->buckets[hash_val] = new_entry;

	/* add entry in list */
	list_add_before(&hash->entries, &new_entry->node);
	return 0;
}

int futils_hash_insert(struct hash *hash, uint32_t key, void *data)
{
	struct hash_entry *entry;
	int ret;

	if (!hash)
		return -EINVAL;

	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return -ENOMEM;

	entry->is_const = 0;
	entry->data = data;
	entry->key = key;
	entry->next = NULL;

	ret = futils_hash_insert_entry(hash, key, entry);
	if (ret < 0)
		free(entry);

	return ret;
}

int futils_hash_insert_const(struct hash *hash, uint32_t key,
			     const void *data)
{
	struct hash_entry *entry;
	int ret;

	if (!hash)
		return -EINVAL;

	entry = calloc(1, sizeof(*entry));
	if (!entry)
		return -ENOMEM;

	entry->is_const = 1;
	entry->const_data = data;
	entry->key = key;
	entry->next = NULL;

	ret = futils_hash_insert_entry(hash, key, entry);
	if (ret < 0)
		free(entry);

	return ret;
}

static int futils_hash_lookup_entry(const struct hash *tab, uint32_t key,
				    struct hash_entry **_entry)
{
	struct hash_entry *entry;
	uint32_t hash;

	if (!tab || !_entry)
		return -EINVAL;

	/* compute entry hash from key and find entry from buckets */
	hash = hash_32(key);
	hash = hash % tab->size;
	entry = tab->buckets[hash];

	/* compare keys to find correct entry */
	while (entry && entry->key != key)
		entry = entry->next;

	/* entry not found */
	if (!entry)
		return -ENOENT;

	*_entry = entry;
	return 0;
}

int futils_hash_lookup(const struct hash *hash, uint32_t key, void **data)
{
	struct hash_entry *entry;
	int ret;

	ret = futils_hash_lookup_entry(hash, key, &entry);
	if (ret < 0)
		return ret;

	if (entry->is_const)
		return -EPERM;

	if (data)
		*data = entry->data;

	return 0;
}

int futils_hash_lookup_const(const struct hash *hash, uint32_t key,
			     const void **data)
{
	struct hash_entry *entry;
	int ret;

	ret = futils_hash_lookup_entry(hash, key, &entry);
	if (ret < 0)
		return ret;

	if (data)
		*data = entry->const_data;

	return 0;
}

int futils_hash_remove(struct hash *tab, uint32_t key)
{
	struct hash_entry *entry, *prev;
	uint32_t hash;

	if (!tab)
		return -EINVAL;

	/* compute entry hash from key and find entry from buckets */
	hash = hash_32(key);
	hash = hash % tab->size;
	entry = tab->buckets[hash];

	/* compare keys to find correct entry */
	prev = NULL;
	while (entry && entry->key != key) {
		prev = entry;
		entry = entry->next;
	}

	/* entry not found */
	if (!entry)
		return -ENOENT;

	/* remove entry */
	if (!prev)
		tab->buckets[hash] = entry->next;
	else
		prev->next = entry->next;

	list_del(&entry->node);
	free(entry);
	return 0;
}

