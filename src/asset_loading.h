void ParseSoundConfig(token *config, const char *config_file);
void ReadInSoundsFromDirectory(dir_folder *sound_folder);
void SoundDirCallback(dir_folder *folder, char *filename);

void ParseShaderConfig(shader *s, token *config);
void ReadInShadersFromDirectory(dir_folder *shader_folder);
void ShaderDirCallback(dir_folder *folder, char *filename);

void ReadInTexturesFromDirectory(dir_folder *texture_folder);
void TextureDirCallback(dir_folder *folder, char *filename);

void ReadInFontsFromDirectory(dir_folder *folder);

struct image_file
{
	int w;
	int h;
	int n;
	uint8_t *image_data;
};

image_file LoadImageFile(char *fileName);
void FreeImageFile(image_file *img);


struct async_load_image_result
{
	image_file *files;
	int numFiles;
	generic_callback *callback;
	void *callerData;
};

//creates a message that is put on the message queue, which will receive the data from the result, and the callback to send the data to
void AsyncLoadImageFiles(char **fileNames, int numFiles, generic_callback *callback, void *callerData);
