enum game_flags
{
	GAME_OBJECT			= 0x1,
};

struct cmp_metadata
{
	uint32_t cmpInUse;
	uint32_t flags;
};

#define UPDATE_FUNCTION(Name) void Name(int goId)
typedef UPDATE_FUNCTION(update_function);

struct game_function
{
	char *name;
	update_function *function;
};
global_variable game_function GameFunctions[1000];
global_variable int NumGameFunctions;

update_function *GetFunction(char *functionName)
{
	for (int i = 0; i < NumGameFunctions; i++)
	{
		if (strcmp(GameFunctions[i].name, functionName) == 0)
			return GameFunctions[i].function;
	}
	assert(false);
	return NULL;
}

#define MAX_GAME_OBJECTS 10000
#include "auto_component.h"

int GetFirstOfType(object_type type)
{
	int result = -1;
	for (int i = 0; i < NumGameObjects; i++)
	{
		if (GameComponents.type[i] == type)
		{
			result = GameComponents.idIndex[i];
			break;
		}
	}
	return result;
}

global_variable int ObjectsToRemove[MAX_GAME_OBJECTS];
global_variable int NumObjectsToRemove;

inline void RemoveObject(int id)
{
	ObjectsToRemove[NumObjectsToRemove++] = id;
}

void ProcessObjectRemovals()
{
	for (int i = 0; i < NumObjectsToRemove - 1; i++)
	{
		for (int j = 0; j < NumObjectsToRemove - i - 1; j++)
		{
			if (ObjectsToRemove[j] < ObjectsToRemove[j+1])
			{
				int tmp = ObjectsToRemove[j];
				ObjectsToRemove[j] = ObjectsToRemove[j+1];
				ObjectsToRemove[j+1] = tmp;
			}
		}
	}

	for (int i = 0; i < NumObjectsToRemove; i++)
	{
		_RemoveObject(ObjectsToRemove[i]);
	}
	NumObjectsToRemove = 0;
}

