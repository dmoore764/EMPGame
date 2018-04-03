struct uid_entry
{
	uint32_t id;
	void *data;
};

struct uid_map
{
	bool internal_storage;
	size_t object_size;
	uint32_t next_uid;
	int maxCount;
	int numEntries;
	uid_entry *table;
};

void InitUIDMap(uid_map *map, int size, bool allocate_internal_storage = false, size_t object_space = 0);
void DestroyUIDMap(uid_map *map);
bool UIDMapIsValid(uid_map *map);

uint32_t UIDMapAddCopyToInternalStorage(uid_map *map, void *data);
uint32_t UIDMapAddData(uid_map *map, void *data);
uid_entry *UIDAddEntry(uid_map *map);

int _UIDGetTableIndex(uid_map *map, uint32_t uid);
void *UIDMapGet(uid_map *map, uint32_t uid);
void *UIDMapRemove(uid_map *map, uint32_t uid);
