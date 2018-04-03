#define MAX_HASH_ITERATIONS 10

struct hash_item
{
	union
	{
		size_t index;
		char *key;
		glm::ivec2 location;
	};

	void *data;

	hash_item *next;
};

struct generic_hash
{
	bool initialized;
	int table_size;
	hash_item **items;
};


void	   InitHash(generic_hash *hash, size_t table_size, memory_arena *arena = NULL);
void       DestroyHash(generic_hash *hash, memory_arena *arena = NULL);

void	   *GetFromHash(generic_hash *hash, size_t index);
void	   *GetFromHash(generic_hash *hash, const char *key);
void	   *GetFromHash(generic_hash *hash, glm::ivec2 location);

void	   RemoveFromHash(generic_hash *hash, size_t index);
void	   RemoveFromHash(generic_hash *hash, const char *key);
void	   RemoveFromHash(generic_hash *hash, glm::ivec2 location);

void	   AddToHash(generic_hash *hash, void *data, size_t index, memory_arena *arena = NULL);
void	   AddToHash(generic_hash *hash, void *data, const char *key, memory_arena *arena = NULL);
void	   AddToHash(generic_hash *hash, void *data, glm::ivec2 location, memory_arena *arena = NULL);

uint32_t   HashIndexForString(const char *str);

size_t HashGetFirst(generic_hash* hash, char** key, size_t* index, void** value);
bool HashGetNext(generic_hash* hash, char** key, size_t* index, void** value, size_t iterationId);
void HashEndIteration(size_t iterationId);
