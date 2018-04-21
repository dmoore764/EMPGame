#pragma once

/*
struct PrefabInfo
{
	char editorDisplayName[64];
	bool canRotate;
	bool canScale;

	json_hash_element *placingComponents;
};

struct MemberInfo
{
	char name[64];
	char type[64];
	void *location;
	bool isArray;
	int count;
};
*/

#define FW(format, ...) {fprintf(file, format, ##__VA_ARGS__);fprintf(file,"\n");}

/*void SerializeObject(char *result, uint32_t componentsToSerialize, int goId)
{
	sprintf(result, "{ ");
	char temp[1024];
	auto meta = GO(metadata);
	if ((componentsToSerialize & TRANSFORM) && meta->cmpInUse & TRANSFORM)
	{
		auto cmp = GO(transform);
		sprintf(temp, "transform : { ");
		strcat(result, temp);

		sprintf(temp, "pos : [ %d, %d ], ", cmp->pos.x, cmp->pos.y);
		strcat(result, temp);

		strcat(temp, " }, ");
	}
}*/

void SerializeLevel(bool *savedLevel, char *levelName)
{
	char levelFullPath[1024];
	if (!(*savedLevel))
	{
		dir_folder levelFolder = LoadDirectory(App.baseFolder, "levels");
		int levelNum = 1;
		char levelFile[64];
		sprintf(levelFile, "Level_%d.json", levelNum);
		while (GetFileInDirectory(&levelFolder, false, levelFile))
		{
			levelNum++;
			sprintf(levelFile, "Level_%d.json", levelNum);
		}

		strcpy(levelName, levelFile);
		*savedLevel = true;
	}

	char temp[1024];
	CombinePath(temp, App.baseFolder, "levels");
	CombinePath(levelFullPath, temp, levelName);

	int startX = MAP_W;
	int startY = MAP_H;
	int endX = 0;
	int endY = 0;

	for (int x = 0; x < MAP_W; x++)
	{
		for (int y = 0; y < MAP_H; y++)
		{
			if (TileMapV[y*MAP_W + x])
			{
				if (x < startX)
					startX = x;
				if (y < startY)
					startY = y;
				if (x >= endX)
					endX = x+1;
				if (y >= endY)
					endY = y+1;
			}
		}
	}

	int pitch = endX - startX;

	FILE *file = fopen(levelFullPath, "w");
	if (file)
	{
		FW("{");

		FW("    InitialCamera: [ %d, %d ],", InitialCameraPos.x, InitialCameraPos.y);

		FW("    CameraRestraints: [");
		for (int i = 0; i < NumCameraRestraintAreas; i++)
		{
			irect *area = &CameraRestraintAreas[i];
			FW("        [[%d,%d],[%d,%d]],", area->UR.x, area->UR.y, area->BL.x, area->BL.y);
		}
		FW("	],");

		FW("    Objects: [");

		for (int i = 0; i < NumLevelObjects; i++)
		{
			char objectSaveData[2048];
			SerializeObject(objectSaveData, TRANSFORM | STRING_STORAGE | WAYPOINTS, LevelObjects[i].objectID);
			FW("        [ \"%s\", %s ],", LevelObjects[i].prefab, objectSaveData);
		}

		FW("    ],");

		FW("    TileX: %d,", startX);
		FW("    TileY: %d,", startY);
		FW("    Pitch: %d,", pitch);
		FW("    Tiles: ");
		FW("    [");
		fprintf(file, "        ");

		for (int y = startY; y < endY; y++)
		{
			for (int x = startX; x < endX; x++)
			{
				fprintf(file, "0x%x,", TileMapV[y*MAP_W + x]);
				fprintf(file, "%d,", TileMapI[y*MAP_W + x]);
			}
			fprintf(file, "\n        ");
		}
		FW("    ]");
		FW("}");

		fclose(file);
	}
}

//Deserialize
//GoRef MakePrefab(const char *prefab_name);
//GoRef DeserializeJSON(json_value *val);
//bool DeserializeComponent(GoRef go, json_hash_element *el);

//void SerializeObject(GoRef go, FILE *file);
//void SerializeComponent(CmpRef cmp, FILE *file, int indentLevel = 1);

//bool ComponentGetNextMember(CmpRef cmp, int *number, MemberInfo *info);
