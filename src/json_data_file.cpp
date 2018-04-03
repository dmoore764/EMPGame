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
			dat->file = file;

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
