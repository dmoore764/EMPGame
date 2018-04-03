#pragma once

#include <new>

template <class T>
class IdPool {
public:
	// Creates a new IdMap with the given capacity
	IdPool(size_t capacity);

	// Destroys an IdMap
	~IdPool();

	// Gets the maximum number of items stored in the pool
	inline size_t GetMaxCount() { return maxCount; }

	// Gets the number of free items in the pool
	inline size_t GetFreeCount() { return freeCount; }

	// Gets the number of used items in the pool
	inline size_t GetUsedCount() { return maxCount - freeCount; }

	// Create a new ID that maps to the given data pointer.
	// The map does not take ownership of the data or make a copy, it just stores an association
	size_t Allocate(T** data = NULL);

	// Checks if the given Id is currently valid (maps to some data) or invalid (part of the free entry list)
	bool IdIsValid(size_t id);

	// Gets the data associated with a valid ID
	inline T* Get(size_t id) {
		// Assert that the ID is valid
		assert(IdIsValid(id));

		// Return associated data
		return &table[id - 1].data;
	}

	// Remove the association between an ID and its data, and free that ID up for reuse
	// Returns the data for convenience so that IdMapGetData doesn't need to be called before removing data
	void Deallocate(size_t id);

private:
	// A single entry in the ID map data table.
	// Entries that are in use point to user data, while entries that are free point to the next free entry or null.
	struct IdEntry {
		union {
			// The next free entry in the list, or NULL if this is the last entry
			IdEntry* nextFree;

			// The data stored in the entry
			T data;
		};

		IdEntry() {}
		~IdEntry() {}
	};

	// The total number of Ids available to be assigned
	size_t maxCount;

	// The total number of Ids free for use
	size_t freeCount;

	// A table holding all the data for the map.
	IdEntry* table;

	// The head of the free entry list
	IdEntry* nextFree;

	// The last free entry in the list
	IdEntry* lastFree;
};

template <class T>
IdPool<T>::IdPool(size_t capacity) {
	maxCount = capacity;
	freeCount = capacity;

	// Get memory for the table
	table = (IdEntry*)malloc(sizeof(IdEntry) * capacity);

	// Set each entry in the table to point to the next entry
	for (size_t i = 0; i < capacity - 1; ++i) {
		table[i].nextFree = &table[i + 1];
	}

	// Set final node to null
	table[capacity - 1].nextFree = NULL;

	// Save head and tail of list
	nextFree = table;
	lastFree = &table[capacity - 1];
}

template <class T>
IdPool<T>::~IdPool() {
	// Free table memory
	free(table);
}

template <class T>
size_t IdPool<T>::Allocate(T** data) {
	// Assert that we have a free entry
	assert(freeCount > 0);

	// Get the next free entry
	IdEntry* entry = nextFree;

	// Remove the entry from the free list
	nextFree = entry->nextFree;

	// Decrement free count
	freeCount--;

	// Deal with case where all nodes are used
	if (lastFree == entry) {
		lastFree = NULL;
	}

	// Send out pointer to new data, if requested
	if (data != NULL) {
		*data = &entry->data;
	}

	// Use placement new to call constructor on data
	new (&entry->data) T();

	// Set the associated into the entry and use pointer arithmetic to return its position in the table array
	// Add one so that 0 is reserved as NULL
	return entry - table + 1;
}

template <class T>
bool IdPool<T>::IdIsValid(size_t id) {
	// Assert that ID is in range of the collection
	assert(id <= maxCount);

	// Shift id back once since we reserved zero
	id--;

	// Doesn't work anymore since the object in the pool is no longer a void pointer to an address outside the pool
	//// Check if the pointer stored in the entry points to a memory location within the table array
	//// This means that it is part of the free list, and thus not in use
	//size_t offset = table[id].nextFree - nextFree;
	//if (offset < maxCount) {
	//	return false;
	//}

	//// Check if this is the last ID in the free list (which will point to NULL but is still free)
	//if (&table[id] == lastFree) {
	//	return false;
	//}

	// The ID is in use
	return true;
}

template <class T>
void IdPool<T>::Deallocate(size_t id) {
	// Assert that the ID is valid
	assert(IdIsValid(id));

	// Shift id back once since we reserved zero
	id--;

	// Call the desctructor on the data pointed to
	table[id].data.~T();

	// Deal with the case where there were no remaining free Ids
	if (lastFree == NULL) {
		lastFree = &table[id];
		nextFree = lastFree;
	}
	else {
		lastFree->nextFree = &table[id];
		lastFree = &table[id];
	}

	// Set next free on new final item in list to NULL, overwriting previously stored data association
	lastFree->nextFree = NULL;

	// Increment free count
	freeCount++;
}