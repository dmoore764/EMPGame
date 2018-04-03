#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "memory.h"

#include "assert.h"
#include "string.h"

#include "errno.h"
#include "../src/string_utilities.h"

#ifdef __APPLE__
	// POSIX
	#include "unistd.h"	
	#include "libproc.h"
	#include "dirent.h"

	//Posix thread stuff
	#include "pthread.h"

	//mac thing to get the api for reading the mac directory
#define Component AppleComponent
	#include <CoreServices/CoreServices.h>
#undef Component

	#include <OpenGL/gl3.h>
	#include <OpenGL/gl3ext.h>

#else
	#include <GL/glew.h>
	// Windows OS header
	#define WIN32_LEAN_AND_MEAN
	#include "Windows.h"
#endif

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../src/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../src/stb_truetype.h"

#define KILOBYTES(Number) (1024L*Number)
#define MEGABYTES(Number) (1024L*KILOBYTES(Number))
#define GIGABYTES(Number) (1024L*MEGABYTES(Number))

#define MAKE_COLOR(r,g,b,a) ((a << 24) | (b << 16) | (g << 8) | r)
#define ArrayCount(array) (sizeof(array)/sizeof(array[0]))

//static has 3 different meanings
#define global_variable static
#define internal_function static
#define local_persistent static



#include "../src/file_utilities.h"
#include "../src/debug_output.h"
#include "../src/memory_arena.h"
#include "../src/tokenizer.h"
#include "../src/hash.h"
#include "../src/directory_helper.h"
#include "../src/opengl.h"
#include "../src/asset_loading.h"
#include "../src/rectangle_packing.h"
#include "../src/fonts.h"
#include "../src/render_commands.h"
#include "../src/json_data_file.h"

#include "SDL.h"

#define FW(format, ...) {fprintf(file, format, ##__VA_ARGS__);fprintf(file,"\n");}

size_t ToCamelCase(char* dst, char* src, bool capitalizeFirstLetter);
int strupr(char *dst, char *src);
void strlwr(char *dst, char *src);

global_variable memory_arena Arena;
global_variable char *BaseFolder;
global_variable generic_hash Components;
global_variable generic_hash Prefabs;
global_variable generic_hash Objects;

#include "../src/file_utilities.cpp"
#include "../src/debug_output.cpp"
#include "../src/memory_arena.cpp"
#include "../src/tokenizer.cpp"
#include "../src/hash.cpp"
#include "../src/directory_helper.cpp"
#include "../src/json_data_file.cpp"

void PrintCStruct(json_data_file *cmp, FILE *file)
{
	assert(cmp->val->type == JSON_HASH);
	char className[128];
	strlwr(className, cmp->file->basename);
	char cmpName[128];
	sprintf(cmpName, "cmp_%s", className);

	FW("struct %s", cmpName);
	FW("{");

	int numSerializedMembers = 0;
	{
		json_value *serializedMembers = cmp->val->hash->GetByKey("serialized");
		if (serializedMembers)
		{
			if (serializedMembers->type != JSON_HASH)
			{
				PrintErrorAtToken(serializedMembers->start->tokenizer_copy, "Members specified as hash { \"member_name\" : {\"type\": \"float\", ...}");
				assert(false);
			}
			json_hash_element *item = serializedMembers->hash->first;
			if (item)
				FW("    //serialized members");

			while (item)
			{
				if (item->value->type != JSON_HASH)
				{
					PrintErrorAtToken(item->value->start->tokenizer_copy, "Members specified as hash { \"member_name\" : {\"type\": \"float\", ...}");
					assert(false);
				}
				json_value *type = item->value->hash->GetByKey("type");
				if (type->type != JSON_STRING)
				{
					PrintErrorAtToken(item->value->start->tokenizer_copy, "Types are expressed like so: {\"type\": \"float\", ...}");
					assert(false);
				}
				

				// Optional arraySize argument
				json_value *arraySize = item->value->hash->GetByKey("count");
				if (arraySize != NULL)
				{
					if (arraySize->type == JSON_STRING)
					{
						numSerializedMembers++;
						FW("    %s %s[%s];", type->string, item->key, arraySize->string);
					}
					else if (arraySize->type == JSON_NUMBER)
					{
						numSerializedMembers++;
						FW("    %s %s[%lld];", type->string, item->key, arraySize->number.i);
					}
					else
					{
						PrintErrorAtToken(item->value->start->tokenizer_copy, "Array Sizes are expressed like so: {\"count\": \"CONSTANT_SIZE\", ...}");
						assert(false);
					}
				}
				else
				{
					numSerializedMembers++;
					FW("    %s %s;", type->string, item->key);
				}

				item = item->next;
			}
		}
	}

	{
		json_value *nonSerializedMembers = cmp->val->hash->GetByKey("nonSerialized");
		if (numSerializedMembers && nonSerializedMembers)
			FW("");

		if (nonSerializedMembers)
		{
			if (nonSerializedMembers->type != JSON_HASH)
			{
				PrintErrorAtToken(nonSerializedMembers->start->tokenizer_copy, "Members specified as hash { \"member_name\" : {\"type\": \"float\", ...}");
				assert(false);
			}
			json_hash_element *item = nonSerializedMembers->hash->first;
			if (item)
				FW("    //nonSerialized members");

			while (item)
			{
				if (item->value->type != JSON_HASH)
				{
					PrintErrorAtToken(item->value->start->tokenizer_copy, "Members specified as hash { \"member_name\" : {\"type\": \"float\", ...}");
					assert(false);
				}
				json_value *type = item->value->hash->GetByKey("type");
				if (type->type != JSON_STRING)
				{
					PrintErrorAtToken(item->value->start->tokenizer_copy, "Types are expressed like so: {\"type\": \"float\", ...}");
					assert(false);
				}
				

				// Optional arraySize argument
				json_value *arraySize = item->value->hash->GetByKey("count");
				if (arraySize != NULL)
				{
					if (arraySize->type == JSON_STRING)
					{
						numSerializedMembers++;
						FW("    %s %s[%s];", type->string, item->key, arraySize->string);
					}
					else if (arraySize->type == JSON_NUMBER)
					{
						numSerializedMembers++;
						FW("    %s %s[%lld];", type->string, item->key, arraySize->number.i);
					}
					else
					{
						PrintErrorAtToken(item->value->start->tokenizer_copy, "Array Sizes are expressed like so: {\"count\": \"CONSTANT_SIZE\", ...}");
						assert(false);
					}
				}
				else
				{
					numSerializedMembers++;
					FW("    %s %s;", type->string, item->key);
				}

				item = item->next;
			}
		}
	}

	FW("};");
	FW("");
	FW("");

}

void PrintDefaultLoad(json_hash_element *item, FILE *file, int indentSpaces, bool defineAndInitialize)
{
	if(item->value->type != JSON_HASH)
	{
		PrintErrorAtToken(item->value->start->tokenizer_copy, "default values should be specified as a hash {\"member_name\" : 1.5");
		assert(false);
	}
	json_value *default_val = item->value->hash->GetByKey("default");
	json_value *type = item->value->hash->GetByKey("type");
	json_value *count = item->value->hash->GetByKey("count");
	int fixedArraySize = 0;
	if(type->type != JSON_STRING)
	{
		PrintErrorAtToken(item->value->start->tokenizer_copy, "default values should be specified as a hash {\"member_name\" : 1.5");
		assert(false);
	}


	if (count)
	{
		if (count->type != JSON_NUMBER)
		{
			PrintErrorAtToken(count->start->tokenizer_copy, "fixed size array types are specified by a \"count\" parameter with a integer value");
			assert(false);
		}
		fixedArraySize = GetJSONValAsUint32(count);
	}

	char typeName[128];
	sprintf(typeName, "%s%s", defineAndInitialize ? type->string : "", defineAndInitialize ? " " : "");

	char indent[32];
	sprintf(indent, "%.*s", indentSpaces, "                               ");

	char fixedArray[16];
	if (count && defineAndInitialize)
		sprintf(fixedArray, "[%d]", fixedArraySize);
	else
		sprintf(fixedArray, "");

	if (default_val)
	{

		if (strcmp(type->string, "float") == 0 && !count)
		{
			float val = GetJSONValAsFloat(default_val);
			FW("%smember->%s%s%s = %ff;", indent, typeName, item->key, fixedArray, val);
		}
		else if (strcmp(type->string, "size_t") == 0 && !count)
		{
			uint32_t val = GetJSONValAsUint32(default_val);
			FW("%smember->%s%s%s = %u;", indent, typeName, item->key, fixedArray, val);
		}
		else if (strcmp(type->string, "int") == 0 && !count)
		{
			int val = GetJSONValAsInt(default_val);
			FW("%smember->%s%s%s = %d;", indent, typeName, item->key, fixedArray, val);
		}
		else if (strcmp(type->string, "uint16_t") == 0 && !count)
		{
			uint16_t val = GetJSONValAsUint32(default_val);
			FW("%smember->%s%s%s = %d;", indent, typeName, item->key, fixedArray, val);
		}
		else if (strcmp(type->string, "bool") == 0 && !count)
		{
			bool val = GetJSONValAsBool(default_val);
			FW("%smember->%s%s%s = %s;", indent, typeName, item->key, fixedArray, val ? "true" : "false");
		}
		else if (strcmp(type->string, "vec2") == 0 && !count)
		{
			if (default_val->type != JSON_ARRAY || default_val->array->num_elements != 2)
			{
				PrintErrorAtToken(default_val->start->tokenizer_copy, "vec2 default values specified as arrays [200.0, 323.39]");
				assert(false);
			}
			json_value *val0 = default_val->array->GetByIndex(0);
			json_value *val1 = default_val->array->GetByIndex(1);
			float x = GetJSONValAsFloat(val0);
			float y = GetJSONValAsFloat(val1);
			FW("%smember->%s%s%s = vec2(%ff,%ff);", indent, typeName, item->key, fixedArray, x, y);
		}
		else if (strcmp(type->string, "ivec2") == 0 && !count)
		{
			if (default_val->type != JSON_ARRAY || default_val->array->num_elements != 2)
			{
				PrintErrorAtToken(default_val->start->tokenizer_copy, "ivec2 default values specified as arrays [200, 323]");
				assert(false);
			}
			json_value *val0 = default_val->array->GetByIndex(0);
			json_value *val1 = default_val->array->GetByIndex(1);
			int x = GetJSONValAsInt(val0);
			int y = GetJSONValAsInt(val1);
			FW("%smember->%s%s%s = vec2(%d,%d);", indent, typeName, item->key, fixedArray, x, y);
		}
		else if (strcmp(type->string, "uvec2") == 0 && !count)
		{
			if (default_val->type != JSON_ARRAY || default_val->array->num_elements != 2)
			{
				PrintErrorAtToken(default_val->start->tokenizer_copy, "uvec2 default values specified as arrays [200, 323]");
				assert(false);
			}
			json_value *val0 = default_val->array->GetByIndex(0);
			json_value *val1 = default_val->array->GetByIndex(1);
			uint32_t x = GetJSONValAsUint32(val0);
			uint32_t y = GetJSONValAsUint32(val1);
			FW("%smember->%s%s%s = vec2(%d,%d);", indent, typeName, item->key, fixedArray, x, y);
		}
		else if (strcmp(type->string, "uint32_t") == 0 && !count)
		{
			//can specify uint32_t's in various ways
			switch (default_val->type)
			{
				case JSON_ARRAY: //[255,255,255,255]
				{
					if (default_val->array->num_elements != 4)
					{
						PrintErrorAtToken(default_val->start->tokenizer_copy, "uint32_t default value must be array [255,255,255,255] or number 239834 or hex 0xb092304a");
						assert(false);
					}
					json_value *val0 = default_val->array->GetByIndex(0);
					json_value *val1 = default_val->array->GetByIndex(1);
					json_value *val2 = default_val->array->GetByIndex(2);
					json_value *val3 = default_val->array->GetByIndex(3);
					uint32_t r = GetJSONValAsUint8(val0);
					uint32_t g = GetJSONValAsUint8(val1);
					uint32_t b = GetJSONValAsUint8(val2);
					uint32_t a = GetJSONValAsUint8(val3);
					FW("%smember->%s%s%s = ((%d << 24) | (%d << 16) | (%d << 8) | %d);", indent, typeName, item->key, fixedArray, r, g, b, a); 
				} break;

				case JSON_NUMBER:
				{
					uint32_t v = GetJSONValAsUint32(default_val);
					FW("%smember->%s%s%s = %u;", indent, typeName, item->key, fixedArray, v); 
				} break;

				case JSON_STRING:
				{
					if (!(default_val->string[0] == '0' && (default_val->string[1] == 'x' || default_val->string[1] == 'X')))
					{
						PrintErrorAtToken(default_val->start->tokenizer_copy, "Expecting hex value");
						assert(false);
					}
					char *end;
					uint32_t v = strtoul(default_val->string, &end, 16);
					FW("%smember->%s%s%s = %u;", indent, typeName, item->key, fixedArray, v); 
				} break;

				default:
				{
					PrintErrorAtToken(default_val->start->tokenizer_copy, "uint32_t default value must be array [255,255,255,255] or number 239834 or hex 0xb092304a");
					assert(false);
				} break;
			}
		}
		else if (strcmp(type->string, "blend_mode") == 0 && !count)
		{
			assert(default_val->type == JSON_STRING);
			bool found = false;
			for (int i = 0; i < ArrayCount(BlendModeStrings); i++)
			{
				if (strcmp(default_val->string, BlendModeStrings[i]) == 0)
				{
					found = true;
					FW("%smember->%s%s%s = %s;", indent, typeName, item->key, fixedArray, default_val->string);
					break;
				}
			}
			if (!found)
			{
				PrintErrorAtToken(default_val->start->tokenizer_copy, "Did not find blend mode '%s'", default_val->string);
				assert(false);
			}
		}
		else
		{
			if (defineAndInitialize)
			{
				if (default_val->type != JSON_STRING)
				{
					PrintErrorAtToken(default_val->start->tokenizer_copy, "Unknown type, cannot initialize unless the initialized value is passed in as a string");
					assert(false);
				}
				FW("%smember->%s%s%s = %s;", indent, typeName, item->key, fixedArray, default_val->string);
			}
			else if (strchr(type->string, '*'))
			{
				//type is a pointer
				PrintErrorAtToken(type->start->tokenizer_copy, "Cannot initialize a pointer type with a default value (unless it's char *)");
				assert(false);
			}
			else
			{
				if (default_val->type != JSON_STRING)
				{
					PrintErrorAtToken(default_val->start->tokenizer_copy, "Unknown type, cannot initialize unless the initialized value is passed in as a string");
					assert(false);
				}
				FW("%s%s%s%s = %s;", indent, typeName, item->key, fixedArray, default_val->string);
			}
		}
	}
	else if (!count) {
		// If things have no value specified, set them to zero (for types that make sense to do so)
		if (strcmp(type->string, "float") == 0) {
			FW("%smember->%s%s%s = 0.0f;", indent, typeName, item->key, fixedArray);
		} else if (strcmp(type->string, "size_t") == 0) {
			FW("%smember->%s%s%s = 0;", indent, typeName, item->key, fixedArray);
		} else if (strcmp(type->string, "int") == 0) {
			FW("%smember->%s%s%s = 0;", indent, typeName, item->key, fixedArray);
		} else if (strcmp(type->string, "uint32_t") == 0) {
			FW("%smember->%s%s%s = 0;", indent, typeName, item->key, fixedArray);
		} else if (strcmp(type->string, "uint16_t") == 0) {
			FW("%smember->%s%s%s = 0;", indent, typeName, item->key, fixedArray);
		} else if (strcmp(type->string, "bool") == 0) {
			FW("%smember->%s%s%s = false;", indent, typeName, item->key, fixedArray);
		} else if (strcmp(type->string, "blend_mode") == 0) {
			FW("%smember->%s%s%s = BLEND_MODE_ADD;", indent, typeName, item->key, fixedArray);
		} else if (strchr(type->string, '*')) {
			FW("%smember->%s%s%s = NULL;", indent, typeName, item->key, fixedArray);
		} else if (defineAndInitialize) {
			FW("%smember->%s%s%s;", indent, typeName, item->key, fixedArray);
		}
	}
}

int main (int argcount, char **args)
{
	BaseFolder = GetExecutableDir();
	InitArena(&Arena, GIGABYTES(1));
	InitHash(&Components, 64);
	InitHash(&Prefabs, 64);
	InitHash(&Objects, 64);

	dir_folder componentFolder = LoadDirectory(BaseFolder, "components");
	ReadInJsonDataFromDirectory(&componentFolder, &Components);

	dir_folder prefabFolder = LoadDirectory(BaseFolder, "prefabs");
	ReadInJsonDataFromDirectory(&prefabFolder, &Prefabs);

	dir_folder objectFolder = LoadDirectory(BaseFolder, "objects");
	ReadInJsonDataFromDirectory(&objectFolder, &Objects);

	void *tmp;
	HashGetFirst(&Objects, NULL, NULL, &tmp);
	json_data_file *objectTypes = (json_data_file *)tmp;

	char component_h[KILOBYTES(1)];
	CombinePath(component_h, BaseFolder, "src/auto_component.h");

	FILE* file = fopen(component_h, "w");
	if (!file)
	{
		ERR("Could not open file for writing");
		return 1;
	}

	json_data_file *components[64];
	int numComponents = 0;

	{
		void *value;
		size_t iter = HashGetFirst(&Components, NULL, NULL, &value);
		while (value)
		{
			json_data_file *cmp = (json_data_file *)value;
			char file_base[128];
			strupr(file_base, cmp->file->basename);
			components[numComponents++] = cmp;
			HashGetNext(&Components, NULL, NULL, &value, iter);
		}
		HashEndIteration(iter);

		for (int i = 0; i < numComponents-1; i++)
		{
			for (int j = 0; j < numComponents - 1 - j; j++)
			{
				char *s1 = components[j]->val->hash->GetByKey("name")->string;
				char *s2 = components[j+1]->val->hash->GetByKey("name")->string;
				if (strcmp(s1, s2) > 0)
				{
					json_data_file *tmp = components[j];
					components[j] = components[j+1];
					components[j+1] = tmp;
				}
			}
		}
	}


	{
		FW("enum object_type");
		FW("{");
		json_value *item = objectTypes->val->array->first;
		while (item)
		{
			FW("    %s,", item->string);
			item = item->next;
		}
		FW("};");
		FW("");
		FW("");
	}

	//Print out string versions of the enum
	{
		FW("enum component_type {");
		FW("    METADATA,");

		for (int i = 0; i < numComponents; i++)
		{
			FW("    %s = 0x%x,", components[i]->val->hash->GetByKey("name")->string, (1 << i));
		}

		FW("};");
		FW("");
		FW("");
	}

	//Go through and print out all the structs
	{
		for (int i = 0; i < numComponents; i++)
		{
			PrintCStruct(components[i], file);
		}
		FW("");
	}

	//print out the SOA component struct
	{
		FW("struct game_components");
		FW("{");
		FW("    uint32_t idIndex[MAX_GAME_OBJECTS];");
		FW("    object_type type[MAX_GAME_OBJECTS];");
		FW("    cmp_metadata metadata[MAX_GAME_OBJECTS];");

		for (int i = 0; i < numComponents; i++)
		{
			char memberName[128];
			strlwr(memberName, components[i]->file->basename);
			char cmpName[128];
			sprintf(cmpName, "cmp_%s", memberName);
			FW("    %s %s[MAX_GAME_OBJECTS];", cmpName, memberName);
		}

		FW("};");
	}


	{
		FW("global_variable game_components GameComponents;");
		FW("global_variable int NumGameObjects;");
		FW("");
		FW("");
		FW("//the game object is a index into the arrays of components");
		FW("struct GameObjectId");
		FW("{");
		FW("    bool inUse;");
		FW("    uint32_t index;");
		FW("};");
		FW("global_variable GameObjectId GameObjectIDs[MAX_GAME_OBJECTS];");
		FW("");
		FW("//stores the top most used id (could be free spots below this number though)");
		FW("global_variable int idCount;");
		FW("");
		FW("//add an object by finding the first free slot, the returned integer");
		FW("//would never change for the life of the object (but the index into the");
		FW("//gameObjects will)");
		FW("int AddObject(object_type type)");
		FW("{");
		FW("    int result = -1;");
		FW("    for (int i = 0; i < idCount; i++)");
		FW("    {");
		FW("        if (!GameObjectIDs[i].inUse)");
		FW("        {");
		FW("            result = i;");
		FW("            break;");
		FW("        }");
		FW("    }");
		FW("    ");
		FW("    //The list of game objects up to idCount is full, so add at the end");
		FW("    if (result == -1)");
		FW("        result = idCount++;");
		FW("    ");
		FW("    assert(result < MAX_GAME_OBJECTS);");
		FW("    ");
		FW("    GameObjectIDs[result].inUse = true;");
		FW("    GameObjectIDs[result].index = NumGameObjects++;");
		FW("    ");
		FW("    GameComponents.type[NumGameObjects-1] = type;");
		FW("    GameComponents.idIndex[NumGameObjects-1] = result;");
		FW("    GameComponents.metadata[NumGameObjects-1] = {};");

		for (int i = 0; i < numComponents; i++)
		{
			char memberName[128];
			strlwr(memberName, components[i]->file->basename);
			FW("    GameComponents.%s[NumGameObjects-1] = {};", memberName);
		}
		FW("    ");
		FW("    return result;");
		FW("}");
		FW("");
		FW("void _RemoveObject(int id)");
		FW("{");
		FW("    if (id == idCount-1)");
		FW("        idCount--;");
		FW("    ");
		FW("    GameObjectId *goID = &GameObjectIDs[id];");
		FW("    ");
		FW("    if (goID->index != NumGameObjects-1)");
		FW("    {");
		FW("        int idIndex = GameComponents.idIndex[NumGameObjects-1];");
		FW("    ");
		FW("        //move around the objects so the list of components is compact");
		FW("        GameComponents.idIndex[goID->index] = GameComponents.idIndex[NumGameObjects-1];");
		FW("        GameComponents.type[goID->index] = GameComponents.type[NumGameObjects-1];");
		FW("        GameComponents.metadata[goID->index] = GameComponents.metadata[NumGameObjects-1];");

		for (int i = 0; i < numComponents; i++)
		{
			char memberName[128];
			strlwr(memberName, components[i]->file->basename);
			FW("        GameComponents.%s[goID->index] = GameComponents.%s[NumGameObjects-1];", memberName, memberName);
		}
		FW("        ");
		FW("        //if we swapped, update the index into the component lists");
		FW("        GameObjectIDs[idIndex].index = goID->index;");
		FW("    }");
		FW("    ");
		FW("    //the list of object components gets packed down");
		FW("    NumGameObjects--;");
		FW("    ");
		FW("    goID->inUse = false;");
		FW("}");
		FW("");
		FW("");

		FW("#define GO(component) (&GameComponents.component[GameObjectIDs[goId].index])");
		FW("#define OTH(id, component) (&GameComponents.component[GameObjectIDs[id].index])");
		FW("");
		FW("");

	}


	{

		//initializer
		FW("void InitObject(int goId)");
		FW("{");
		FW("    auto meta = GO(metadata);");

		for (int i = 0; i < numComponents; i++)
		{
			char className[128];
			strlwr(className, components[i]->file->basename);

			FW("    if (meta->cmpInUse & %s)", components[i]->val->hash->GetByKey("name")->string);
			FW("    {");
			FW("        auto member = GO(%s);", className);

			json_value *serialized = components[i]->val->hash->GetByKey("serialized");
			if (serialized)
			{
				json_hash_element *item = serialized->hash->first;
				while (item)
				{
					PrintDefaultLoad(item, file, 8, false);
					item = item->next;
				}
			}


			json_value *nonSerialized = components[i]->val->hash->GetByKey("nonSerialized");
			if (nonSerialized)
			{
				json_hash_element *item = nonSerialized->hash->first;
				while (item)
				{
					PrintDefaultLoad(item, file, 8, false);
					item = item->next;
				}
			}

			FW("    }");
		}

		FW("}");

	}


	{
		FW("object_type GetObjectType(char *type)");
		FW("{");
		json_value *item = objectTypes->val->array->first;
		while (item)
		{
			FW("    if (strcmp(type, \"%s\") == 0)", item->string);
			FW("        return %s;", item->string);
			item = item->next;
		}
		FW("    return OBJ_NONE;");
		FW("}");
		FW("");
		FW("");

		FW("component_type GetComponentType(char *type)");
		FW("{");
		for (int i = 0; i < numComponents; i++)
		{
			FW("    if (strcmp(type, \"%s\") == 0)", components[i]->val->hash->GetByKey("name")->string);
			FW("        return %s;", components[i]->val->hash->GetByKey("name")->string);
		}
		FW("    return METADATA;");
		FW("}");
		FW("");
		FW("");
		FW("#include \"serialization_helper.h\"");
		FW("");
		FW("");
		FW("void AnimationUpdate(int goId);");
		FW("");

		FW("int DeserializeObject(json_value *val)");
		FW("{");
		FW("	int goId = AddObject(GetObjectType(val->hash->GetByKey(\"type\")->string));");
		FW("	auto meta = GO(metadata);");
		FW("	json_value *components = val->hash->GetByKey(\"components\");");
		FW("	json_value *item = components->array->first;");
		FW("	while (item)");
		FW("	{");
		FW("		meta->cmpInUse |= GetComponentType(item->string);");
		FW("		item = item->next;");
		FW("	}");
		FW("    InitObject(goId);");
		FW("");
		FW("	json_value *data = val->hash->GetByKey(\"data\");");
		FW("	json_hash_element *el = data->hash->first;");
		FW("");
		FW("	while (el)");
		FW("	{");

		for (int i = 0; i < numComponents; i++)
		{
			char memberName[128];
			strlwr(memberName, components[i]->file->basename);
			char cmpName[128];
			sprintf(cmpName, "cmp_%s", memberName);

			FW("		%sif (strcmp(el->key, \"%s\") == 0)", i > 0 ? "else " : "", memberName);
			FW("		{");
			FW("			auto c = GO(%s);", memberName);
			FW("			json_hash_element *member = el->value->hash->first;");
			FW("			while (member)");
			FW("			{");

			bool firstMember = true;
			json_value *serialized = components[i]->val->hash->GetByKey("serialized");
			if (serialized)
			{
				json_hash_element *item = serialized->hash->first;
				while (item)
				{
					char *type = item->value->hash->GetByKey("type")->string;
					json_value *count = item->value->hash->GetByKey("count");
					FW("				%sif (strcmp(member->key, \"%s\") == 0)", firstMember ? "" : "else ", item->key);
					firstMember = false;
					FW("                {");

					if (strcmp(type, "char *") == 0)
					{
						FW("                    DeserializeCharStar(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "char") == 0 && count)
					{
						FW("                    DeserializeCharArray(member->value, c->%s, %lld);", item->key, count->number.i);
					}
					else if (strcmp(type, "float") == 0 && !count)
					{
						FW("                    DeserializeFloat(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "int") == 0 && !count)
					{
						FW("                    DeserializeInt(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "uint32_t") == 0 && !count)
					{
						FW("                    DeserializeUint32(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "uint16_t") == 0 && !count)
					{
						FW("                    DeserializeUint16(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "bool") == 0 && !count)
					{
						FW("                    DeserializeBool(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "vec2") == 0 && !count)
					{
						FW("                    DeserializeVec2(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "uvec2") == 0 && !count)
					{
						FW("                    DeserializeUvec2(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "ivec2") == 0 && !count)
					{
						FW("                    DeserializeIvec2(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "size_t") == 0 && !count)
					{
						FW("                    DeserializeSizeT(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "update_function *") == 0 && !count)
					{
						FW("                    c->%s = GetFunction(member->value->string);", item->key);
					}
					FW("                }");
					item = item->next;
				}
			}


			json_value *nonSerialized = components[i]->val->hash->GetByKey("nonSerialized");
			if (nonSerialized)
			{
				json_hash_element *item = nonSerialized->hash->first;
				while (item)
				{
					char *type = item->value->hash->GetByKey("type")->string;
					json_value *count = item->value->hash->GetByKey("count");
					FW("				%sif (strcmp(member->key, \"%s\") == 0)", firstMember ? "" : "else ", item->key);
					firstMember = false;
					FW("                {");

					if (strcmp(type, "char *") == 0)
					{
						FW("                    DeserializeCharStar(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "char") == 0 && count)
					{
						FW("                    DeserializeCharArray(member->value, c->%s, %lld);", item->key, count->number.i);
					}
					else if (strcmp(type, "float") == 0 && !count)
					{
						FW("                    DeserializeFloat(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "int") == 0 && !count)
					{
						FW("                    DeserializeInt(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "uint32_t") == 0 && !count)
					{
						FW("                    DeserializeUint32(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "uint16_t") == 0 && !count)
					{
						FW("                    DeserializeUint16(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "bool") == 0 && !count)
					{
						FW("                    DeserializeBool(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "vec2") == 0 && !count)
					{
						FW("                    DeserializeVec2(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "uvec2") == 0 && !count)
					{
						FW("                    DeserializeUvec2(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "ivec2") == 0 && !count)
					{
						FW("                    DeserializeIvec2(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "size_t") == 0 && !count)
					{
						FW("                    DeserializeSizeT(member->value, &c->%s);", item->key);
					}
					else if (strcmp(type, "update_function *") == 0 && !count)
					{
						FW("                    c->%s = GetFunction(member->value->string);", item->key);
					}
					FW("                }");
					item = item->next;
				}
			}

			FW("				member = member->next;");
			FW("			}");
			FW("		}");
		}

		FW("        else { assert(false); } //didn't find member");
		FW("        el = el->next;");
		FW("	}");
		FW("");
		FW("    meta->flags |= GAME_OBJECT;");
		FW("    if ((meta->cmpInUse & ANIM) && (meta->cmpInUse & SPRITE))");
		FW("        AnimationUpdate(goId);");
		FW("");
		FW("	return goId;");
		FW("}");

	}



	fclose(file);

	char component_cpp[KILOBYTES(1)];
	CombinePath(component_cpp, BaseFolder, "src/auto_prefab.h");

	file = fopen(component_cpp, "w");
	if (!file)
	{
		ERR("Could not open file for writing");
		return 1;
	}

	/*
	//Print out the component create function
	{
		FW("")
		FW(" // Component Constructors");
		FW("");

		// Put them in a namespace
		FW("namespace Cmp {");
		FW("");

		void *value;
		size_t iter = HashGetFirst(&Components, NULL, NULL, &value);
		while (value)
		{
			json_data_file *cmp = (json_data_file *)value;
			char *cmp_type = cmp->file->basename;
			char capitalized_type[128];
			char varName[128];
			ToCamelCase(varName, cmp_type, false);
			char className[128];
			ToCamelCase(className, cmp_type, true);
			strupr(capitalized_type, cmp_type);

			FW("%s::%s(Component::Type type, GoRef gameObject, CmpRef self) : Component(type, gameObject, self) {", className, className);
			FW("    //Set default values");
			json_value *serializedMembers = cmp->val->hash->GetByKey("serialized");
			if (serializedMembers)
			{
				if (serializedMembers->type != JSON_HASH)
				{
					PrintErrorAtToken(serializedMembers->start->tokenizer_copy, "Members specified as hash { \"member_name\" : {\"type\": \"float\", ...}");
					assert(false);
				}
				json_hash_element *item = serializedMembers->hash->first;
				while (item)
				{
					PrintDefaultLoad(item, file, 4, false);
					item = item->next;
				}
			}

			json_value *nonSerializedMembers = cmp->val->hash->GetByKey("nonSerialized");
			if (nonSerializedMembers)
			{
				if (nonSerializedMembers->type != JSON_HASH)
				{
					PrintErrorAtToken(nonSerializedMembers->start->tokenizer_copy, "Members specified as hash { \"member_name\" : {\"type\": \"float\", ...}");
					assert(false);
				}
				json_hash_element *item = nonSerializedMembers->hash->first;
				while (item)
				{
					PrintDefaultLoad(item, file, 4, false);
					item = item->next;
				}
			}


			FW("}");
			FW("");

			HashGetNext(&Components, NULL, NULL, &value, iter);
		}
		HashEndIteration(iter);
	}
	*/

	FW("");
	FW("");

	fclose(file);

	char prefab_cpp[KILOBYTES(1)];
	CombinePath(prefab_cpp, BaseFolder, "src/auto_prefab.cpp");
	file = fopen(prefab_cpp, "w");
	if (!file)
	{
		ERR("Could not open file for writing");
		return 1;
	}

	/*
	//Create the auto prefab functions
	{
		FW("#include \"serialization_helper.h\"");

		//Print out the serialization function
		FW("bool ComponentGetNextMember(CmpRef c, int *number, MemberInfo *info)");
		FW("{");
		FW("    int memberNumber = 0;");
		FW("    switch (c->GetType())");
		FW("    {");

		void *value;
		size_t iter = HashGetFirst(&Components, NULL, NULL, &value);
		while (value)
		{
			json_data_file *cmp = (json_data_file *)value;

			char capitalized_type[128];
			char varName[128];
			char className[128];
			char *cmp_type = cmp->file->basename;
			ToCamelCase(varName, cmp->file->basename, false);
			ToCamelCase(className, cmp->file->basename, true);
			strupr(capitalized_type, cmp_type);

			FW("        case Component::%s:", capitalized_type);
			FW("        {");
			FW("            auto cmp = Component::From<Cmp::%s>(c);", className);

			json_value *serializedMembers = cmp->val->hash->GetByKey("serialized");
			if (serializedMembers)
			{
				json_hash_element *item = serializedMembers->hash->first;
				while (item)
				{
					char *name = item->key;
					json_value *type = item->value->hash->GetByKey("type");
					json_value *count = item->value->hash->GetByKey("count");
					FW("            if (memberNumber++ == *number)");
					FW("            {");
					FW("                *number = *number + 1;");
					FW("                strcpy(info->name, \"%s\");", name);
					FW("                strcpy(info->type, \"%s\");", type->string);
					FW("                info->location = &cmp->%s;", name)
					FW("                info->isArray = %s;", count ? "true" : "false");
					if (count)
						FW("                info->count = %lld;", count->number.i);
					FW("                return true;");
					FW("            }");

					item = item->next;
				}
			}

			//Probably shouldn't display these in the editor since they won't be saved anyway
			json_value *nonSerializedMembers = cmp->val->hash->GetByKey("nonSerialized");
			if (nonSerializedMembers)
			{
				json_hash_element *item = nonSerializedMembers->hash->first;
				while (item)
				{
					char *name = item->key;
					json_value *type = item->value->hash->GetByKey("type");
					json_value *count = item->value->hash->GetByKey("count");
					FW("            if (memberNumber++ == *number)");
					FW("            {");
					FW("                *number = *number + 1;");
					FW("                strcpy(info->name, \"%s\");", name);
					FW("                strcpy(info->type, \"%s\");", type->string);
					FW("                info->isArray = %s;", count ? "true" : "false");
					if (count)
						FW("                info->count = %d;", count->number.i);
					FW("                return true;");
					FW("            }");

					item = item->next;
				}
			}


			FW("        } break;");
			FW("");

			HashGetNext(&Components, NULL, NULL, &value, iter);
		}
		HashEndIteration(iter);

		FW("        case Component::TYPE_COUNT: {} break;");

		FW("    }");
		FW("    return false;");
		FW("}");
		FW("");


		FW("");
		FW("");
		FW("void SerializeComponent(CmpRef c, FILE *file, int indentLevel)");
		FW("{");
		FW("    switch (c->GetType())");
		FW("    {");

		iter = HashGetFirst(&Components, NULL, NULL, &value);
		while (value)
		{
			json_data_file *cmp = (json_data_file *)value;

			char capitalized_type[128];
			char varName[128];
			char className[128];
			char *cmp_type = cmp->file->basename;
			ToCamelCase(varName, cmp->file->basename, false);
			ToCamelCase(className, cmp->file->basename, true);
			strupr(capitalized_type, cmp_type);

			FW("        case Component::%s:", capitalized_type);
			FW("        {");
			FW("            auto cmp = Component::From<Cmp::%s>(c);", className);
			FW("            FPrintIndent(file, indentLevel);");
			FW("            fprintf(file, \"%s : {\\n\");", className);

			json_value *serializedMembers = cmp->val->hash->GetByKey("serialized");
			if (serializedMembers)
			{
				json_hash_element *item = serializedMembers->hash->first;
				while (item)
				{
					char *name = item->key;
					json_value *type = item->value->hash->GetByKey("type");
					json_value *count = item->value->hash->GetByKey("count");
					if (strcmp(type->string, "vec2") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: [%%f, %%f],\\n\", cmp->%s.x, cmp->%s.y);", name, name, name);
					}
					else if (strcmp(type->string, "float") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: %%f,\\n\", cmp->%s);", name, name);
					}
					else if (strcmp(type->string, "int") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: %%d,\\n\", cmp->%s);", name, name);
					}
					else if (strcmp(type->string, "uint32_t") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: 0x%%x,\\n\", cmp->%s);", name, name);
					}
					else if (strcmp(type->string, "bool") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: %%s,\\n\", cmp->%s ? \"true\" : \"false\");", name, name);
					}
					else if (strcmp(type->string, "blend_mode") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: %%s,\\n\", BlendModeStrings[(int)(cmp->%s)]);", name, name);
					}
					else if (strcmp(type->string, "char *") == 0 || strcmp(type->string, "char*") == 0)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: \\\"%%s\\\",\\n\", cmp->%s);", name, name);
					}
					else if (strcmp(type->string, "char") == 0 && count)
					{
						FW("            FPrintIndent(file, indentLevel + 1);");
						FW("            fprintf(file, \"%s: \\\"%%s\\\",\\n\", cmp->%s);", name, name);
					}

					item = item->next;
				}
			}


			FW("        } break;");
			FW("");

			HashGetNext(&Components, NULL, NULL, &value, iter);
		}
		HashEndIteration(iter);

		FW("        case Component::TYPE_COUNT: {} break;");

		FW("    }");
		FW("    FPrintIndent(file, indentLevel);");
		FW("    fprintf(file, \"},\\n\");");
		FW("}");
		FW("");


		FW("");
		FW("bool DeserializeComponent(GoRef go, json_hash_element *el)");
		FW("{");

		size_t cmpiter = HashGetFirst(&Components, NULL, NULL, &value);
		while (value)
		{
			json_data_file *cmp = (json_data_file *)value;
			char file_base[128];
			ToCamelCase(file_base, cmp->file->basename, true);

			FW("    if (strcmp(el->key, \"%s\") == 0)", file_base);
			FW("    {");
			FW("        auto c = go->Add<Cmp::%s>();", file_base);
			FW("");
			FW("        json_value *members = el->value;");
			FW("        if (members->type != JSON_HASH)");
			FW("        {");
			FW("            PrintErrorAtToken(el->keyword->tokenizer_copy, \"Members of this component are specified in a hash\");");
			FW("            return false;");
			FW("        }");
			FW("");


			json_value *serializedMembers = cmp->val->hash->GetByKey("serialized");
			if (serializedMembers)
			{
				json_hash_element *item = serializedMembers->hash->first;
				while (item)
				{

					FW("        {");
					FW("            json_value *member = members->hash->GetByKey(\"%s\");", item->key);
					FW("            if (member)");
					FW("            {");
					char *type = item->value->hash->GetByKey("type")->string;
					json_value *count = item->value->hash->GetByKey("count");
					if (strcmp(type, "char *") == 0)
					{
						FW("                if (!DeserializeCharStar(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "char") == 0 && count)
					{
						FW("                if (!DeserializeCharArray(member, c->%s, %lld)) { return false; }", item->key, count->number.i);
					}
					else if (strcmp(type, "float") == 0)
					{
						FW("                if (!DeserializeFloat(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "int") == 0)
					{
						FW("                if (!DeserializeInt(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "uint32_t") == 0)
					{
						FW("                if (!DeserializeUint32(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "bool") == 0)
					{
						FW("                if (!DeserializeBool(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "vec2") == 0)
					{
						FW("                if (!DeserializeVec2(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "size_t") == 0)
					{
						FW("                if (!DeserializeSizeT(member, &c->%s)) { return false; }", item->key);
					}

					FW("            }");
					FW("        }");

					item = item->next;
				}
			}

			json_value *nonSerializedMembers = cmp->val->hash->GetByKey("nonSerialized");
			if (nonSerializedMembers)
			{
				json_hash_element *item = nonSerializedMembers->hash->first;
				while (item)
				{

					FW("        {");
					FW("            json_value *member = members->hash->GetByKey(\"%s\");", item->key);
					FW("            if (member)");
					FW("            {");
					char *type = item->value->hash->GetByKey("type")->string;
					if (strcmp(type, "char *") == 0)
					{
						FW("                if (!DeserializeCharStar(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "float") == 0)
					{
						FW("                if (!DeserializeFloat(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "int") == 0)
					{
						FW("                if (!DeserializeInt(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "uint32_t") == 0)
					{
						FW("                if (!DeserializeUint32(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "bool") == 0)
					{
						FW("                if (!DeserializeBool(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "vec2") == 0)
					{
						FW("                if (!DeserializeVec2(member, &c->%s)) { return false; }", item->key);
					}
					else if (strcmp(type, "size_t") == 0)
					{
						FW("                if (!DeserializeSizeT(member, &c->%s)) { return false; }", item->key);
					}

					FW("            }");
					FW("        }");

					item = item->next;
				}
			}



			FW("    }");
			HashGetNext(&Components, NULL, NULL, &value, cmpiter);
		}
		HashEndIteration(cmpiter);


		FW("    return true;");
		FW("}");
		FW("");

	}*/
	fclose(file);

	return 0;
}
