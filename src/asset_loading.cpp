/*

//Basic data about how to load and play sounds
//The sounds specified in the file must be in the same directory or sub-directories (otherwise they won't be loaded yet)
//Format: <SoundFile> (filename only no extension) <loops> (loop/no_loop) <default volume> <channel>
//Example

Town	loop		1.0		CHANNEL_MUSIC
Snarl	no_loop		0.5		CHANNEL_EFFECTS

*/
void ParseSoundConfig(token *config, const char *config_file)
{
	while(config)
	{
		assert(config && config->type == TOKEN_TYPE_WORD);
		char token_text[128];
		sprintf(token_text, "%.*s", config->length, config->text);
		sound_file *sf = (sound_file *)GetFromHash(&Sounds, token_text);
		if (sf)
		{
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_WORD);

			if (TokenMatches(config, "loop"))
			{
				if (sf->loops != true)
				{
					UnloadSoundResource(sf);
					sf->loops = true;
				}
			}
			else if (TokenMatches(config, "no_loop"))
			{
				if (sf->loops != false)
				{
					UnloadSoundResource(sf);
					sf->loops = false;
				}
			}
			else
			{
				ERR("Invalid token '%.*s' in sound config file %s.  Expecting 'loop' or 'no_loop'", config->length, config->text, config_file);
				return;
			}

			config = config->next;

			assert(config && config->type == TOKEN_TYPE_NUMBER);
			sf->default_volume = (float)atof(config->text);
			config = config->next;

			assert(config && config->type == TOKEN_TYPE_WORD);
			bool found_channel = false;
			for (int i = 0; i < ArrayCount(SoundChannelNames); i++)
			{
				if (TokenMatches(config, SoundChannelNames[i]))
				{
					found_channel = true;
					sf->default_channel = (sound_channel)i;
				}
			}
			if (!found_channel)
			{
				DBG("Did not find channel named '%.*s' in sound config file %s", config->length, config->text, config_file);
			}

			config = config->next;
		}
		else
		{
			ERR("Invalid token '%s' in sound config file %s (missing sound, or sound in parent directory from config file?).  Cannot parse rest of file", token_text, config_file);
			return;
		}
	}
}

void ReadInSoundsFromDirectory(dir_folder *sound_folder)
{
	void *value;
	if (size_t iter = HashGetFirst(&sound_folder->folders, NULL, NULL, &value))
	{
		do
		{
			dir_folder *sub_folder = (dir_folder *)value;
			ReadInSoundsFromDirectory(sub_folder);
		} while (HashGetNext(&sound_folder->folders, NULL, NULL, &value, iter));
		HashEndIteration(iter);
	}

	if (size_t iter = HashGetFirst(&sound_folder->files, NULL, NULL, &value))
	{
		do
		{
			dir_file *file = (dir_file *)value;

			if (strcmp(file->ext, "ogg") == 0)
			{
				sound_file *sf = (sound_file *)GetFromHash(&Sounds, file->basename);
				bool new_file = sf == NULL;

				if (sf == NULL)
				{
					sf = PUSH_ITEM(&Arena, sound_file);
					strcpy(sf->name, file->basename);

					AddToHash(&Sounds, sf, file->basename);
				}

				if (file->modified && !new_file)
				{
					//unload old sound
					UnloadSoundResource(sf);
				}
				else if (new_file)
				{
					char src_file[KILOBYTES(1)];
					sprintf(src_file, "%s%s", file->folder_path, file->name);

					sf->loaded = false;
					sf->sample = NULL;
					sprintf(sf->filename, src_file);
					sf->loops = false;
					sf->default_volume = 1.0f;
					sf->last_time_played = 0;
					sf->default_channel = CHANNEL_MAIN;

					//if the word "music" is in the path somewhere, 
					//then we will assume this is a music file
					if (strstr(file->folder_path, "music"))
					{
						sf->is_music = true;
						sf->default_channel = CHANNEL_MUSIC;
					}
					else
						sf->is_music = false;
				}

				file->modified = false;
			}
		} while (HashGetNext(&sound_folder->files, NULL, NULL, &value, iter));
		HashEndIteration(iter);
	}

	//Go through files one more time and parse any config files in this sound directory
	if (size_t iter = HashGetFirst(&sound_folder->files, NULL, NULL, &value)) {
		do
		{
			dir_file *file = (dir_file *)value;
			file->modified = false;
			if (strcmp(file->ext, "txt") == 0)
			{
				char src_file[KILOBYTES(1)];
				sprintf(src_file, "%s%s", file->folder_path, file->name);
				char *config_src = ReadFileIntoString(src_file);
				if (config_src)
				{
					transient_arena transient = GetTransientArena(&Arena);
					token *config_tokens = Tokenize(config_src, src_file, &Arena);
					ParseSoundConfig(config_tokens, src_file);
					FreeTransientArena(transient);
				}
			}
		} while (HashGetNext(&sound_folder->files, NULL, NULL, &value, iter));
		HashEndIteration(iter);
	}
}


void SoundDirCallback(dir_folder *folder, char *filename)
{	
	strcpy(SoundFileToReload, filename);
	ReloadSounds = true;
}


void ParseShaderConfig(shader *s, token *config)
{
	while(config)
	{
		assert(config && config->type == TOKEN_TYPE_WORD);
		if (TokenMatches(config, "has_vertex_color_attribute"))
		{
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_COLON);
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_WORD);
			s->uses_color = TokenMatches(config, "true");
			config = config->next;
		}
		else if (TokenMatches(config, "has_vertex_uv_attribute"))
		{
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_COLON);
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_WORD);
			s->uses_uv = TokenMatches(config, "true");
			config = config->next;
		}
		else if (TokenMatches(config, "number_of_tex_units"))
		{
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_COLON);
			config = config->next;
			assert(config && config->type == TOKEN_TYPE_NUMBER);
			s->number_of_tex_units = atoi(config->text);
			config = config->next;
		}
	}

	s->number_of_attributes = 1;
	//position
	vertex_attribute *attr = &s->attributes[0];
	attr->single_unit_size = sizeof(float);
	attr->number_of_units = 3;
	attr->type = GL_FLOAT;

	if (s->uses_color)
	{
		attr = &s->attributes[s->number_of_attributes++];
		attr->single_unit_size = sizeof(uint8_t);
		attr->number_of_units = 4;
		attr->type = GL_UNSIGNED_BYTE;
	}

	if (s->uses_uv)
	{
		attr = &s->attributes[s->number_of_attributes++];
		attr->single_unit_size = sizeof(float);
		attr->number_of_units = 2;
		attr->type = GL_FLOAT;
	}

	s->vertex_stride = 0;
	for (int i = 0; i < s->number_of_attributes; i++)
		s->vertex_stride += s->attributes[i].single_unit_size * s->attributes[i].number_of_units;
}

void ReadInShadersFromDirectory(void *datapak)
{
	table_of_contents *toc = (table_of_contents *)datapak;
	for (int i = 0; i < toc->numRecords; i++)
	{
		file_record *fr = &toc->records[i];
		if (strcmp(fr->folderName, "shaders") == 0)
		{
			if (strcmp(fr->fileName + strlen(fr->fileName) - 4, "vert") == 0)
			{
				char basename[64];
				sprintf(basename, "%.*s", strlen(fr->fileName) - 5, fr->fileName);

				shader *s = (shader *)GetFromHash(&Shaders, basename);
				if (s == NULL)
				{
					s = PUSH_ITEM(&Arena, shader);
					strcpy(s->name, basename);

					AddToHash(&Shaders, s, basename);
				}

				char src_file[KILOBYTES(1)];

				//parse the config file for this shader
				sprintf(src_file, "%s.txt", basename);
				file_record *configFile = GetFileRecordWithName(toc, src_file);

				char *config_src = GetFileFromRecordsAsString(datapak, configFile);
				if (config_src)
				{
					transient_arena transient = GetTransientArena(&Arena);
					token *config_tokens = Tokenize(config_src, src_file, &Arena);
					ParseShaderConfig(s, config_tokens);
					FreeTransientArena(transient);

					char *basic_vertsrc = GetFileFromRecordsAsString(datapak, fr);
					sprintf(src_file, "%s.frag", basename);
					file_record *fragsrc = GetFileRecordWithName(toc, src_file);
					char *basic_fragsrc = GetFileFromRecordsAsString(datapak, fragsrc);
					CompileProgram(s, basename, basic_vertsrc, basic_fragsrc);
					free(basic_vertsrc);
					free(basic_fragsrc);
				}
			}
		}
	}
}

void ReadInShadersFromDirectory(dir_folder *shader_folder)
{
	for (int i = 0; i < shader_folder->files.table_size; i++)
	{
		hash_item *item = shader_folder->files.items[i];
		while (item)
		{
			dir_file *file = (dir_file *)item->data;
			file->modified = false;
			if (strcmp(file->ext, "vert") == 0)
			{
				shader *s = (shader *)GetFromHash(&Shaders, file->basename);
				if (s == NULL)
				{
					s = PUSH_ITEM(&Arena, shader);
					strcpy(s->name, file->basename);

					AddToHash(&Shaders, s, file->basename);
				}

				char src_file[KILOBYTES(1)];

				//parse the config file for this shader
				sprintf(src_file, "%s%s.txt", file->folder_path, file->basename);
				char *config_src = ReadFileIntoString(src_file);
				if (config_src)
				{
					transient_arena transient = GetTransientArena(&Arena);
					token *config_tokens = Tokenize(config_src, src_file, &Arena);
					ParseShaderConfig(s, config_tokens);
					FreeTransientArena(transient);

					sprintf(src_file, "%s%s", file->folder_path, file->name);
					char *basic_vertsrc = ReadFileIntoString(src_file);
					sprintf(src_file, "%s%s.frag", file->folder_path, file->basename);
					char *basic_fragsrc = ReadFileIntoString(src_file);
					CompileProgram(s, file->basename, basic_vertsrc, basic_fragsrc);
					free(basic_vertsrc);
					free(basic_fragsrc);
				}

			}
			item = item->next;
		}
	}
}

//This is the function that is sent to the thread that looks for file changes.
//When the callback function gets called (i.e., when a file has changed), 
//This function gets sent the dir_folder that is being watched and the filename
//of the file that has changed (although that's not used in this case, because
//all shaders are recompiled every time a single change is made) - we can change
//this later potentially.  If we make that change, that's fine, but the filename
//currently only works for files in the folder we originally specified, 
//not sub-directories
void ShaderDirCallback(dir_folder *folder, char *filename)
{
	RecompileShaders = true;
}

void ParseSpriteInfo(char *config_src)
{
	assert(config_src);

	token *config_tokens = Tokenize(config_src, "sprite descriptions", &Arena);
	json_value *sprVal = ParseJSONValue(&config_tokens);

	if (sprVal->type != JSON_ARRAY)
	{
		PrintErrorAtToken(sprVal->start->tokenizer_copy, "Sprite description files should be an array");
		assert(false);
	}

	json_value *item = sprVal->array->first;

	while (item)
	{
		if (item->type != JSON_HASH)
		{
			PrintErrorAtToken(item->start->tokenizer_copy, "Each entry should be a hash");
			assert(false);
		}

		json_value *tex = item->hash->GetByKey("sprite");
		if (!tex)
		{
			PrintErrorAtToken(item->start->tokenizer_copy, "Each entry needs a 'sprite' field");
			assert(false);
		}
		game_sprite *s = (game_sprite *)GetFromHash(&Sprites, tex->string);

		json_value *origin = item->hash->GetByKey("origin");
		if (!origin)
		{
			PrintErrorAtToken(item->start->tokenizer_copy, "Each entry needs an 'origin' field");
			assert(false);
		}

		JsonErrorUnlessVec2(origin);
		s->origin = {GetJSONValAsFloat(origin->array->GetByIndex(0)), s->tex_rect->h - GetJSONValAsFloat(origin->array->GetByIndex(1))};

		json_value *POIs = item->hash->GetByKey("POIs");
		if (POIs)
		{
			json_hash_element *el = POIs->hash->first;
			int counter = 0;
			while (el && counter < ArrayCount(s->POIs))
			{
				poi *p = &s->POIs[s->numPOIs++];
				strcpy(p->name, el->key);
				json_value *point = el->value;
				JsonErrorUnlessVec2(point);
				p->location = {GetJSONValAsInt(point->array->GetByIndex(0)), s->tex_rect->h - GetJSONValAsInt(point->array->GetByIndex(1))};
				p->location -= s->origin;

				counter++;
				el = el->next;
			}
		}


		bool colliderIsOffset = false;
		json_value *isOffset = item->hash->GetByKey("colliderIsOffset");
		if (isOffset)
			colliderIsOffset = GetJSONValAsBool(isOffset);

		json_value *collider = item->hash->GetByKey("collider");
		if (collider)
		{
			json_data_file *col = (json_data_file *)GetFromHash(&ColliderDescriptions, collider->string);
			if (!col)
			{
				PrintErrorAtToken(collider->start->tokenizer_copy, "No collider found by this name");
				assert(false);
			}

			json_value *mask = col->val->hash->GetByKey("collision mask");
			if (mask)
			{
				s->numColliders = 0;

				if (mask->type != JSON_ARRAY)
				{
					PrintErrorAtToken(mask->start->tokenizer_copy, "Collision masks are arrays of collision structure hashes. Aborting parsing this file.");
					assert(false);
				}

				json_value *mask_part = mask->array->first;

				while (mask_part)
				{
					if (mask_part->type != JSON_HASH)
					{
						PrintErrorAtToken(mask_part->start->tokenizer_copy, "Collision shape is a hash { type: poly, points: [ [0,0], [200,100], ...] }");
						assert(false);
					}
					CollisionObject shape = {};
					json_value *type = mask_part->hash->GetByKey("type");

					if (!type || type->type != JSON_STRING)
					{
						PrintErrorAtToken(mask_part->start->tokenizer_copy, "Collision shape must have type specified \"type\": \"poly\" (or circle, point)");
						assert(false);
					}

					if (strcmp(type->string, "poly") == 0)
					{
						shape.type = CollisionObject::POLYGON;

						json_value *points = mask_part->hash->GetByKey("points");
						if (!points || points->type != JSON_ARRAY)
						{
							PrintErrorAtToken(mask_part->start->tokenizer_copy, "Polygon colliders must have a list of points");
							assert(false);
						}

						json_value *p = points->array->first;
						while (p)
						{
							JsonErrorUnlessVec2(p);
							vec2 point = {GetJSONValAsFloat(p->array->GetByIndex(0)), GetJSONValAsFloat(p->array->GetByIndex(1))};
							if (colliderIsOffset)
								point = s->origin - point;
							else
								point = vec2(point.x, s->tex_rect->h - point.y);

							PolygonAddVertex(&shape.poly, point);
							p = p->next;
						}

					}
					else if (strcmp(type->string, "circle") == 0)
					{
						shape.type = CollisionObject::CIRCLE;

						json_value *center = mask_part->hash->GetByKey("center");
						JsonErrorUnlessVec2(center);
						shape.circle.center = {GetJSONValAsFloat(center->array->GetByIndex(0)), s->tex_rect->h - GetJSONValAsFloat(center->array->GetByIndex(1))};

						json_value *radius = mask_part->hash->GetByKey("radius");
						if (!radius || radius->type != JSON_NUMBER)
						{
							PrintErrorAtToken(mask_part->start->tokenizer_copy, "Radius must be specified \"radius\": 50");
							assert(false);
						}
						shape.circle.radius = GetJSONValAsFloat(radius);
					}

					CollisionObject *collider = &s->colliders[s->numColliders++];
					*collider = shape;

					mask_part = mask_part->next;
				}
			}
		}
		item = item->next;
	}

}

void ReadInTileTexturesFromDirectory(void *datapak)
{
	table_of_contents *toc = (table_of_contents *)datapak;

	for (int i = 0; i < toc->numRecords; i++)
	{
		file_record *fr = &toc->records[i];
		if (strcmp(fr->folderName, "tile_textures") == 0)
		{
			char basename[64];
			sprintf(basename, "%.*s", strlen(fr->fileName) - 4, fr->fileName);
			char ext[4];
			sprintf(ext, "%s", fr->fileName + strlen(fr->fileName) - 3);

			if (strcmp(ext, "png") == 0)
			{
				game_tile_sprites *t = (game_tile_sprites *)GetFromHash(&TileSprites, basename);
				if (t == NULL)
				{
					size_t fileLength;
					void *fileData = GetBinaryFileFromRecords(datapak, fr, &fileLength);
					t = AddTileSpritesToAvailableAtlas(&SpritePack, basename, fileData, fileLength, true);
				}
			}
		}
	}
}

void ReadInTileTexturesFromDirectory(dir_folder *tile_texture_folder)
{
	void *value;
	size_t iter = HashGetFirst(&tile_texture_folder->files, NULL, NULL, &value);
	while (value)
	{
		dir_file *file = (dir_file *)value;

		if (strcmp(file->ext, "png") == 0)
		{
			game_tile_sprites *t = (game_tile_sprites *)GetFromHash(&TileSprites, file->basename);
			bool loadtex = (t == NULL) || file->modified;
			file->modified = false;
			if (loadtex)
			{
				char src_file[KILOBYTES(1)];
				sprintf(src_file, "%s%s", file->folder_path, file->name);
				size_t fileLength;
				void *fileData = ReadBinaryFile(src_file, &fileLength);
				t = AddTileSpritesToAvailableAtlas(&SpritePack, file->basename, fileData, fileLength, true);
				free(fileData);
			}
		}
		HashGetNext(&tile_texture_folder->files, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);
}



void ReadInTexturesFromDirectory(void *datapak)
{
	table_of_contents *toc = (table_of_contents *)datapak;

	for (int i = 0; i < toc->numRecords; i++)
	{
		file_record *fr = &toc->records[i];
		if (strcmp(fr->folderName, "textures") == 0)
		{
			char basename[64];
			sprintf(basename, "%.*s", strlen(fr->fileName) - 4, fr->fileName);
			char ext[4];
			sprintf(ext, "%s", fr->fileName + strlen(fr->fileName) - 3);

			if (strcmp(ext, "png") == 0)
			{
				game_sprite *s = (game_sprite *)GetFromHash(&Sprites, basename);

				if (s == NULL)
				{
					size_t fileLength;
					void *fileData = GetBinaryFileFromRecords(datapak, fr, &fileLength);
					s = AddSpriteToAvailableAtlas(&SpritePack, basename, fileData, fileLength, true);
					s->origin = {};
					s->numColliders = 0;
				}
			}
		}
	}

	for (int i = 0; i < toc->numRecords; i++)
	{
		file_record *fr = &toc->records[i];
		if (strcmp(fr->folderName, "textures") == 0)
		{
			char basename[64];
			sprintf(basename, "%.*s", strlen(fr->fileName) - 5, fr->fileName);
			char ext[5];
			sprintf(ext, "%s", fr->fileName + strlen(fr->fileName) - 4);

			if (strcmp(ext, "json") == 0)
			{
				char *config_src = GetFileFromRecordsAsString(datapak, fr);
				ParseSpriteInfo(config_src);
			}
		}
	}
}


void ReadInTexturesFromDirectory(dir_folder *texture_folder)
{
	void *value;
	size_t iter = HashGetFirst(&texture_folder->folders, NULL, NULL, &value);
	while (value)
	{
		dir_folder *sub_folder = (dir_folder *)value;
		ReadInTexturesFromDirectory(sub_folder);
		HashGetNext(&texture_folder->folders, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);

	iter = HashGetFirst(&texture_folder->files, NULL, NULL, &value);
	while (value)
	{
		dir_file *file = (dir_file *)value;

		if (strcmp(file->ext, "png") == 0)
		{
			//texture *t = (texture *)GetFromHash(&Textures, file->basename);
			game_sprite *s = (game_sprite *)GetFromHash(&Sprites, file->basename);

			//determine whether we actually need to load the file
			bool loadtex = ((s == NULL) || file->modified);
			file->modified = false;

			if (loadtex)
			{
				char src_file[KILOBYTES(1)];
				sprintf(src_file, "%s%s", file->folder_path, file->name);
				size_t fileLength;
				void *fileData = ReadBinaryFile(src_file, &fileLength);
				s = AddSpriteToAvailableAtlas(&SpritePack, file->basename, fileData, fileLength, true);
				free(fileData);
				s->origin = {};
				s->numColliders = 0;
			}
		}
		HashGetNext(&texture_folder->files, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);



	iter = HashGetFirst(&texture_folder->files, NULL, NULL, &value);
	while (value)
	{
		dir_file *file = (dir_file *)value;

		if (strcmp(file->ext, "json") == 0)
		{
			char src_file[KILOBYTES(1)];
			sprintf(src_file, "%s%s", file->folder_path, file->name);
			char *config_src = ReadFileIntoString(src_file);
			ParseSpriteInfo(config_src);
		}
		HashGetNext(&texture_folder->files, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);
}

//Same as above, except we check in the folder (not subdirectories currently)
//to see if we already have a texture for the file, if we do, then we set that
//file to "modified", so that only one texture is updated.  If we didn't find the
//file, then in the function above (ReadInTexturesFromDirectory) won't find the
//texture in the Hash, so it will be created.  In other words, we can drag textures
//into the directory and they will be ready in the game
void TextureDirCallback(dir_folder *folder, char *filename)
{	
	strcpy(TextureFileToReload, filename);
	ReloadTextures = true;
}


void ReadInFontsFromDirectory(void *datapak)
{
	table_of_contents *toc = (table_of_contents *)datapak;
	for (int i = 0; i < toc->numRecords; i++)
	{
		file_record *fr = &toc->records[i];
		if (strcmp(fr->folderName, "fonts") == 0)
		{
			char basename[64];
			sprintf(basename, "%.*s", strlen(fr->fileName) - 4, fr->fileName);
			char ext[4];
			sprintf(ext, "%s", fr->fileName + strlen(fr->fileName) - 3);
			if (strcmp(ext, "ttf") == 0 || strcmp(ext, "otf") == 0)
			{

				font *newfont = PUSH_ITEM(&Arena, font);

				size_t fileLength;
				void *fileData = GetBinaryFileFromRecords(datapak, fr, &fileLength);
				InitFont(newfont, basename, fileData, fileLength);
				AddToHash(&Fonts, newfont, basename);
			}
		}
	}
}

void ReadInFontsFromDirectory(dir_folder *folder)
{
	void *value;
	if (size_t iter = HashGetFirst(&folder->folders, NULL, NULL, &value))
	{
		do
		{
			dir_folder *sub_folder = (dir_folder *)value;
			ReadInFontsFromDirectory(sub_folder);

		} while (HashGetNext(&folder->folders, NULL, NULL, &value, iter));
		HashEndIteration(iter);
	}

	if (size_t iter = HashGetFirst(&folder->files, NULL, NULL, &value))
	{
		do
		{
			dir_file *file = (dir_file *)value;

			if (strcmp(file->ext, "ttf") == 0 || strcmp(file->ext, "otf") == 0)
			{

				font *newfont = PUSH_ITEM(&Arena, font);

				char src_file[KILOBYTES(1)];
				sprintf(src_file, "%s%s", file->folder_path, file->name);
				size_t fileLength;
				void *fileData = ReadBinaryFile(src_file, &fileLength);

				InitFont(newfont, file->basename, fileData, fileLength);

				if (strstr(file->folder_path, "japanese_fonts") != NULL)
					newfont->language = font::JAPANESE;
				else
					newfont->language = font::ENGLISH;
				AddToHash(&Fonts, newfont, file->basename);
			}
		} while (HashGetNext(&folder->files, NULL, NULL, &value, iter));
		HashEndIteration(iter);
	}
}

image_file LoadImageFile(char *filename)
{
	image_file result;
	
	int n, w, h;
	uint8_t *image_data = stbi_load(filename, &w, &h, &n, 0);
	
	result.w = w;
	result.h = h;
	result.n = n;
	result.image_data = image_data;

	return result;
}

void FreeImageFile(image_file *img)
{
	stbi_image_free(img->image_data);
	img->image_data = NULL;
	img->w = 0;
	img->h = 0;
}

struct _async_load_image_data
{
	char **fileNames;
	int numFiles;
	generic_callback *callback;
	void *callerData;
};

void *_AsyncLoadImageFileThreadEntry(void *ptr)
{
	_async_load_image_data *data = (_async_load_image_data *)ptr;

	async_load_image_result *result = (async_load_image_result *)malloc(sizeof(async_load_image_result));
	result->callback = data->callback;
	result->numFiles = data->numFiles;
	result->files = (image_file *)malloc(sizeof(image_file)*data->numFiles);
	result->callerData = data->callerData;
	
	for (int i = 0; i < data->numFiles; i++)
	{
		int n, w, h;
		uint8_t *image_data = stbi_load(data->fileNames[i], &w, &h, &n, 0);

		result->files[i].w = w;
		result->files[i].h = h;
		result->files[i].n = n;
		result->files[i].image_data = image_data;
	}

	dds_message msg;
	msg.type = DDS_IMAGE_FILES_LOADED;
	msg.data = (void *)result;
	AddMessage(&App.msgQueue, msg);

	for (int i = 0; i < data->numFiles; i++)
	{
		free (data->fileNames[i]);
	}
	free (data->fileNames);
	free(data);
	return NULL;
}

void AsyncLoadImageFiles(char **fileNames, int numFiles, generic_callback *callback, void *callerData)
{
	_async_load_image_data *data = (_async_load_image_data *)malloc(sizeof(_async_load_image_data));
	data->fileNames = (char **)malloc(sizeof(char *)*numFiles);
	for (int i = 0; i < numFiles; i++)
	{
		data->fileNames[i] = (char *)malloc(strlen(fileNames[i]+1));
		strcpy(data->fileNames[i], fileNames[i]);
	}
	data->numFiles = numFiles;
	data->callback = callback;
	data->callerData = callerData;

	pthread_t thread1;
	int iret1;
	iret1 = pthread_create(&thread1, NULL, _AsyncLoadImageFileThreadEntry, (void*)data);
	if(iret1)
	{
		ERR("Error - pthread_create() return code: %d", iret1);
	}
}
