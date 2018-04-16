struct json_data_file
{
	char *baseName;
	char *fileName;
	json_value *val;
	void *additional_data;
};

void ReadInJsonDataFromDirectory(dir_folder *folder, generic_hash *storage);
