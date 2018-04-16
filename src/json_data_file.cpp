void ReadInJsonDataFromDirectory(void *datapak, const char *directory, generic_hash *storage)
{
	table_of_contents *toc = (table_of_contents *)datapak;
	for (int i = 0; i < toc->numRecords; i++)
	{
		file_record *fr = &toc->records[i];
		
		if (strcmp(directory, fr->folderName) == 0)
		{
			char basename[64];
			sprintf(basename, "%.*s", strlen(fr->fileName) - 5, fr->fileName);
			char ext[5];
			sprintf(ext, "%s", fr->fileName + strlen(fr->fileName) - 4);

			if (strcmp(ext, "json") == 0)
			{
				json_data_file *dat = (json_data_file *)GetFromHash(storage, basename);
				if (!dat)
				{
					dat = PUSH_ITEM(&Arena, json_data_file);
					AddToHash(storage, dat, basename);
				}
				dat->fileName = fr->fileName;
				char *basenamecpy = PUSH_ARRAY(&Arena, char, 64);
				strcpy(basenamecpy, basename);
				dat->baseName = basenamecpy;

				char *src = GetFileFromRecordsAsString(datapak, fr);
				token *tok = Tokenize(src, fr->fileName, &Arena);
				json_value *datVal = ParseJSONValue(&tok);
				dat->val = datVal;
			}
		}
	}
}

void ReadInJsonDataFromDirectory(dir_folder *folder, generic_hash *storage)
{
	void *value;
	size_t iter = HashGetFirst(&folder->folders, NULL, NULL, &value);
	while (value)
	{
		dir_folder *sub_folder = (dir_folder *)value;
		ReadInJsonDataFromDirectory(sub_folder, storage);
		HashGetNext(&folder->folders, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);

	iter = HashGetFirst(&folder->files, NULL, NULL, &value);
	while (value)
	{
		dir_file *file = (dir_file *)value;

		if (strcmp(file->ext, "json") == 0)
		{	
			json_data_file *dat = (json_data_file *)GetFromHash(storage, file->basename);
			if (!dat)
			{
				dat = PUSH_ITEM(&Arena, json_data_file);
				AddToHash(storage, dat, file->basename);
			}
			dat->fileName = file->name;
			dat->baseName = file->basename;

			char src_file[KILOBYTES(1)];
			sprintf(src_file, "%s%s", file->folder_path, file->name);
			char *src = ReadFileIntoString(src_file);
			token *tok = Tokenize(src, src_file, &Arena);
			json_value *datVal = ParseJSONValue(&tok);
			dat->val = datVal;
		}

		HashGetNext(&folder->files, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);

}
