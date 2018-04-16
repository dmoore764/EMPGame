#pragma once

struct file_record
{
	char fileName[64];
	char folderName[64];
	size_t length;
	size_t offset;
};

struct table_of_contents
{
	int numRecords;
	file_record records[1000];
};


void *GetBinaryFileFromRecords(void *data, file_record *fr, size_t *length)
{
	void *result = (char *)data + sizeof(table_of_contents) + fr->offset;
	*length = fr->length;
	return result;
}

char *GetFileFromRecordsAsString(void *data, file_record *fr)
{
	size_t length;
	void *filedata = GetBinaryFileFromRecords(data, fr, &length);
	char *result = (char *)malloc(length+1);
	memcpy(result, filedata, length);
	result[length] = '\0';
	return result;
}


file_record *GetFileRecordWithName(table_of_contents *toc, const char *fileName)
{
	for (int i = 0; i < toc->numRecords; i++)
	{
		if (strcmp(toc->records[i].fileName, fileName) == 0)
		{
			return &toc->records[i];
		}
	}
	return NULL;
}

