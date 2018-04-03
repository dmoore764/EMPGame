#include "id_pool.h"

// Hide internal stuff with a namespace
namespace _Hash {
	struct hash_iterator {
		size_t table_index;
		hash_item *item;
	};

	IdPool<hash_iterator> iteratorPool(MAX_HASH_ITERATIONS);
}

// http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/
int IsPowerOfTwo(size_t x)
{
	return ((x != 0) && ((x & (~x + 1)) == x));
}

void InitHash(generic_hash *hash, size_t table_size, memory_arena *arena)
{
	if (!IsPowerOfTwo(table_size))
	{
		ERR("Hash tables must be a power of 2\n");
		assert(false);
	}

	hash->initialized = true;
	hash->table_size = table_size;

	if (arena)
		hash->items = PUSH_ARRAY(arena, hash_item *, table_size);
	else
		hash->items = (hash_item **)calloc(sizeof(hash_item *)*table_size, 1);
}

void _FreeHashItemsRecursive(hash_item* item) {
	if (item == NULL) {
		return;
	}

	// Recurse all the way down to the last item;
	if (item->next != NULL) {
		_FreeHashItemsRecursive(item->next);
	}

	// Delete the items on the way back up
	free(item);
}

void DestroyHash(generic_hash *hash, memory_arena *arena) {
	if (arena) {
		// ???
	} else {
		// Free items
		for (size_t i = 0; i < hash->table_size; ++i) {
			_FreeHashItemsRecursive(hash->items[i]);
		}
	}
	hash->initialized = false;
}

void *GetFromHash(generic_hash *hash, size_t index)
{
	assert(hash->initialized);
	int hash_index = index & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	hash_item *result = hash->items[hash_index];
	while (result)
	{
		if (result->index == index)
			return result->data;
		result = result->next;
	}
	return NULL;
}

uint32_t HashIndexForString(const char *str)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void *GetFromHash(generic_hash *hash, const char *key)
{
	assert(hash->initialized);
	uint32_t hash_index = HashIndexForString(key) & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	hash_item *result = hash->items[hash_index];
	while (result)
	{
		if (strcmp(result->key, key) == 0)
			return result->data;
		result = result->next;
	}
	return NULL;
}

uint32_t HashIndexForIVec2(ivec2 location)
{
    int hash = 23;
	hash = hash * 31 + location.x;
	hash = hash * 31 + location.y;
    return hash;
}

void *GetFromHash(generic_hash *hash, ivec2 location)
{
	assert(hash->initialized);
	uint32_t hash_index = HashIndexForIVec2(location) & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	hash_item *result = hash->items[hash_index];
	while (result)
	{
		if (result->location == location)
			return result->data;
		result = result->next;
	}
	return NULL;
}

void RemoveFromHash(generic_hash *hash, size_t index)
{
	assert(hash->initialized);
	int hash_index = index & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	hash_item **result = &hash->items[hash_index];
	while (*result)
	{
		if ((*result)->index == index)
		{
			(*result) = ((*result)->next);
			return;
		}
		result = &((*result)->next);
	}
}

void RemoveFromHash(generic_hash *hash, const char *key)
{
	assert(hash->initialized);
	uint32_t hash_index = HashIndexForString(key) & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	hash_item **result = &hash->items[hash_index];
	while (*result)
	{
		if (strcmp((*result)->key, key) == 0)
		{
			(*result) = ((*result)->next);
			return;
		}
		result = &((*result)->next);
	}
}

void RemoveFromHash(generic_hash *hash, ivec2 location)
{
	assert(hash->initialized);
	uint32_t hash_index = HashIndexForIVec2(location) & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	hash_item **result = &hash->items[hash_index];
	while (*result)
	{
		if ((*result)->location == location)
		{
			(*result) = ((*result)->next);
			return;
		}
		result = &((*result)->next);
	}
}

void AddToHash(generic_hash *hash, void *data, size_t index, memory_arena *arena)
{
	assert(hash->initialized);
	hash_item *new_item;
	if (arena)
		new_item = PUSH_ITEM(arena, hash_item);
	else
		new_item = (hash_item *)calloc(sizeof(hash_item), 1);

	new_item->index = index;
	new_item->data = data;

	int hash_index = index & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	new_item->next = hash->items[hash_index];
	hash->items[hash_index] = new_item;
}

void AddToHash(generic_hash *hash, void *data, const char *key, memory_arena *arena)
{
	assert(hash->initialized);
	hash_item *new_item;
	if (arena)
	{
		new_item = PUSH_ITEM(arena, hash_item);
		new_item->key = PUSH_ARRAY(arena, char, strlen(key) + 1);
	}
	else
	{
		new_item = (hash_item *)calloc(sizeof(hash_item), 1);
		new_item->key = (char *)calloc(sizeof(char)* (strlen(key) + 1), 1);
	}

	strcpy(new_item->key, key);
	new_item->data = data;

	uint32_t hash_index = HashIndexForString(key) & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	new_item->next = hash->items[hash_index];
	hash->items[hash_index] = new_item;
}

void AddToHash(generic_hash *hash, void *data, ivec2 location, memory_arena *arena)
{
	assert(hash->initialized);
	hash_item *new_item;
	if (arena)
		new_item = PUSH_ITEM(arena, hash_item);
	else
		new_item = (hash_item *)calloc(sizeof(hash_item), 1);

	new_item->location = location;
	new_item->data = data;

	uint32_t hash_index = HashIndexForIVec2(location) & (hash->table_size - 1);
	assert(hash_index >= 0 && hash_index < hash->table_size);

	new_item->next = hash->items[hash_index];
	hash->items[hash_index] = new_item;
}

size_t HashGetFirst(generic_hash* hash, char** key, size_t* index, void** value) {
	using namespace _Hash;

	// Use GetNext to search for the first valid spot in the list
	// Create an iterator pointing to the first table cell, even though it might be empty
	hash_iterator* location;
	size_t iterationId = iteratorPool.Allocate(&location);
	location->table_index = 0;
	location->item = hash->items[location->table_index];

	// Use GetNext to find the first filled in item, if any
	if (location->item != NULL)
	{
		if (key != NULL) {
			*key = location->item->key;
		}
		if (index != NULL) {
			*index = location->item->index;
		}
		if (value != NULL) {
			*value = location->item->data;
		}
	}
	else if (!HashGetNext(hash, key, index, value, iterationId)) {
		// Hash must be empty
		HashEndIteration(iterationId);
		return 0;
	}
	
	// Found the first item
	return iterationId;
}

bool HashGetNext(generic_hash* hash, char** key, size_t* index, void** value, size_t iterationId) {
	using namespace _Hash;

	hash_iterator* location = iteratorPool.Get(iterationId);

	// Check if the item in the iterator has a next value
	if (location->item != NULL && location->item->next != NULL) {
		// Move to the next item in the list
		location->item = location->item->next;
	} else {
		// Search for the next table entry that's used
        while (++location->table_index < hash->table_size && hash->items[location->table_index] == NULL);

		if (location->table_index >= hash->table_size) {
			// Couldn't find any more filled in table entries.
			if (key != NULL) {
				*key = NULL;
			}
			if (index != NULL) {
				*index = 0;
			}
			if (value != NULL) {
				*value = NULL;
			}
			location->item = NULL;
			return false;
		}

		// Found a valid table entry, use first item in its list
		location->item = hash->items[location->table_index];
	}

	// Set out data based on the item that was found
	if (key != NULL) {
		*key = location->item->key;
	}
	if (index != NULL) {
		*index = location->item->index;
	}
	if (value != NULL) {
		*value = location->item->data;
	}

	return location;
}

void HashEndIteration(size_t iterationId) {
	using namespace _Hash;

	// Make it safe to call if iterationID is zero.
	if (iterationId == 0) {
		return;
	}

	// Free the iterator
	iteratorPool.Deallocate(iterationId);
}
