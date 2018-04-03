struct json_data_file
{
	dir_file *file;
	json_value *val;
	void *additional_data;
};

void ReadInJsonDataFromDirectory(dir_folder *folder, generic_hash *storage);
