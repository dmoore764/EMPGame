void InitUIDMap(uid_map *map, int size, bool allocate_internal_storage, size_t object_size)
{
	map->internal_storage = false;
	map->object_size = 0;
	map->next_uid = 1;
	map->maxCount = size;
	map->numEntries = 0;
	map->table = (uid_entry *)malloc(sizeof(uid_entry)*size);
	if (allocate_internal_storage)
	{
		map->internal_storage = true;
		map->object_size = object_size;
		uint8_t *space_for_objects = (uint8_t *)malloc(object_size*size);
		for (int i = 0; i < size; i++)
		{
			map->table[i].data = space_for_objects;
			space_for_objects += object_size;
		}
	}
}

void DestroyUIDMap(uid_map *map)
{
	map->maxCount = 0;
	map->numEntries = 0;
	if (map->internal_storage)
		free(map->table[0].data);
	map->internal_storage = false;
	free(map->table);
	map->table = NULL;
}

bool UIDMapIsValid(uid_map *map)
{
	if (map->table && map->maxCount > 0)
	{
		return true;
	}
	return false;
}

uint32_t UIDMapAddCopyToInternalStorage(uid_map *map, void *data)
{
	assert(map->internal_storage);
	assert(map->numEntries + 1 < map->maxCount);
	uint32_t result = map->next_uid++;
	uid_entry entry = {};
	entry.id = result;
	memcpy(entry.data, data, map->object_size);
	map->table[map->numEntries++] = entry;
	return result;
}

uint32_t UIDMapAddData(uid_map *map, void *data)
{
	assert(map->internal_storage == false);
	assert(map->numEntries + 1 < map->maxCount);
	uint32_t result = map->next_uid++;
	uid_entry entry;
	entry.id = result;
	entry.data = data;
	map->table[map->numEntries++] = entry;
	return result;
}

uid_entry *UIDAddEntry(uid_map *map)
{
	assert(map->internal_storage);
	assert(map->numEntries + 1 < map->maxCount);
	uid_entry *result = &map->table[map->numEntries++];
	result->id = map->next_uid++;
	return result;
}

int _UIDGetTableIndex(uid_map *map, uint32_t uid)
{
	for (int i = 0; i < map->numEntries; i++)
	{
		if (map->table[i].id == uid)
		{
			return i;
		}
	}
	return -1;
}

void *UIDMapGet(uid_map *map, uint32_t uid)
{
	void *result = NULL;
	assert(uid > 0);
	int table_index = _UIDGetTableIndex(map, uid);
	if (table_index > -1)
		result = map->table[table_index].data;
	return result;
}

void *UIDMapRemove(uid_map *map, uint32_t uid)
{
	void *result = NULL;
	int table_index = _UIDGetTableIndex(map, uid);
	if (table_index > -1)
	{
		result = map->table[table_index].data;
		map->table[table_index] = map->table[map->numEntries-1];
		map->numEntries--;
	}
	return result;
}
