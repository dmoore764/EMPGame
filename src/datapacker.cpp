#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "memory.h"
#include "dirent.h"
#include "file_record.h"

static bool PrintOutput;

void *ReadBinaryFile(const char *filename, size_t *length)
{
    void *result = NULL;
    FILE *file = fopen(filename, "r");
    
    if (file) {
        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        *length = size;
        fseek(file, 0L, SEEK_SET);
        
        result = malloc(size);
        fread(result, size, 1, file);
        fclose(file);
    }
    
    return result;
}

void storeDir(table_of_contents *TOC, size_t *currentOffset, void *outputData, const char *originalFolder, const char *name, int indent)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name)))
		return;

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_DIR) {
			char path[1024];
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			//printf("%*s[%s]\n", indent, "", entry->d_name);
			storeDir(TOC, currentOffset, outputData, originalFolder, path, indent + 2);
		} else {
			if (strcmp(entry->d_name, ".DS_Store") == 0)
				continue;

			//printf("%*s- %s\n", indent, "", entry->d_name);
			file_record *fr = &TOC->records[TOC->numRecords++];
			sprintf(fr->fileName, "%s", entry->d_name);
			sprintf(fr->folderName, "%s", originalFolder);

			char fileName[1024];
			sprintf(fileName, "%s/%s", name, entry->d_name);
			size_t length;
			void *data = ReadBinaryFile(fileName, &length);
			fr->length = length;
			fr->offset = *currentOffset;
			*currentOffset += length;

			if (PrintOutput)
				printf("Adding record: %s from folder %s with length %zu at offset %zu\n", fr->fileName, fr->folderName, fr->length, fr->offset);

			memcpy((char *)outputData + fr->offset, data, length);
			free(data);
		}
	}
	closedir(dir);
}

void WriteBinaryFile(const char *filename, void *data, size_t length)
{
    FILE *file = fopen(filename, "w");
	if (file)
	{
		fwrite(data, length, 1, file);
		fclose(file);
	}
}

int main(int argCount, char **args)
{
	if (argCount > 1 && strcmp(args[1], "NO_OUTPUT") == 0)
		PrintOutput = false;
	else
		PrintOutput = true;

	size_t currentOffset = 0;
	void *outputData = calloc(1024*1024*100, 1);
	table_of_contents *TOC = (table_of_contents *)outputData;
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "levels", "../levels", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "prefabs", "../prefabs", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "shaders", "../shaders", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "fonts", "../fonts", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "textures", "../textures", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "tile_textures", "../tile_textures", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "components", "../components", 0);
	storeDir(TOC, &currentOffset, (char *)outputData + sizeof(table_of_contents), "basic_animations", "../basic_animations", 0);

	WriteBinaryFile("binarydata.dat", outputData, sizeof(table_of_contents) + TOC->records[TOC->numRecords - 1].offset + TOC->records[TOC->numRecords - 1].length);

	return 0;
}
