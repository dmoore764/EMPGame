void InitFont(font *f, const char *name, const char *directory, const char *file)
{
	strcpy(f->name, name);
	sprintf(f->filename, "%s%s", directory, file);
	f->font_file_data = (uint8_t *)ReadBinaryFile(f->filename, &f->font_file_length);
	if (!f->font_file_data)
	{
		ERR("Could not find font file '%s'", f->filename);
	}
	else
	{
		stbtt_InitFont(&f->info, f->font_file_data, stbtt_GetFontOffsetForIndex(f->font_file_data,0));
	}
	InitHash(&f->sizes, 16);
}

void GetCodePointMetrics(font_size *fs, int codepoint, font_character *fc)
{
	int advance,lsb,x0,y0,x1,y1;
	stbtt_GetCodepointHMetrics(&fs->f->info, codepoint, &advance, &lsb);
	stbtt_GetCodepointBitmapBox(&fs->f->info,codepoint,fs->scale,fs->scale,&x0,&y0,&x1,&y1);
	fc->codepoint = codepoint;
	fc->advance = advance;
	fc->lsb = lsb;
	fc->x0 = x0;
	fc->x1 = x1;
	fc->y0 = y0;
	fc->y1 = y1;
	fc->move_cursor_x = advance * fs->scale;
}

void ComputeCharacterUV(font_character *fc)
{
	assert(fc->fs);
	assert(fc->atlas_index >= 0 && fc->atlas_index < ArrayCount(fc->fs->atlas));
	assert(fc->fs->atlas[fc->atlas_index].packed);

	float one_over_tex_width = 1 / (float) fc->fs->atlas[fc->atlas_index].w;
	float one_over_tex_height = 1 / (float) fc->fs->atlas[fc->atlas_index].h;
	fc->u0 = fc->tex_rect->x * one_over_tex_width;
	fc->u1 = (fc->tex_rect->x + fc->tex_rect->w) * one_over_tex_width;
	fc->v0 = fc->tex_rect->y * one_over_tex_height;
	fc->v1 = (fc->tex_rect->y + fc->tex_rect->h) * one_over_tex_height;
}

void AddNewAtlasAndTextureToFontSizeRendering(font_size *fs)
{
	fs->current_atlas++;
	int i = fs->current_atlas;
	assert(i < ArrayCount(fs->atlas));

	//Create a new atlas to pack the characters
	InitAtlas(&fs->atlas[i]);
	fs->atlas[i].packed = true;
	fs->atlas[i].outer_border = 0;
	fs->atlas[i].item_border = 2;

	//Determine atlas size based on requested font size and font type (english/non-english)

	switch (fs->f->language)
	{
		case font::ENGLISH:
		{
			if (fs->size <= 24)
			{
				fs->atlas[i].w = 256;
				fs->atlas[i].h = 256;
			}
			else if (fs->size <= 80)
			{
				fs->atlas[i].w = 512;
				fs->atlas[i].h = 512;
			}
			else if (fs->size <= 120)
			{
				fs->atlas[i].w = 1024;
				fs->atlas[i].h = 512;
			}
			else
			{
				fs->atlas[i].w = 1024;
				fs->atlas[i].h = 1024;
			}
		} break;
		case font::JAPANESE:
		{
			fs->atlas[i].w = 1024;
			fs->atlas[i].h = 1024;
		} break;
	}
	
	//Generate the texture space and the GL texture
	fs->rgba_tex_data[i] = PUSH_ARRAY(&Arena, uint8_t, fs->atlas[i].w*4*fs->atlas[i].h);
	memset(fs->rgba_tex_data[i], '\0', fs->atlas[i].w*4*fs->atlas[i].h);

	texture *newtex = PUSH_ITEM(&Arena, texture);
	newtex->width = fs->atlas[i].w;
	newtex->height = fs->atlas[i].h;
	CreateTexture(newtex, NULL, true, true);
	sprintf(newtex->name, "%s %d (atlas %d)", fs->f->name, fs->size, i);
	AddToHash(&Textures, newtex, newtex->name);
	fs->tex[i] = newtex;
}


font_character *AddCharacterToFontSize(font_size *fs, int codepoint, bool render_immediately)
{

	if (fs->current_atlas < 0)
		AddNewAtlasAndTextureToFontSizeRendering(fs);

	font *f = fs->f;
	font_character *fc = (font_character *)GetFromHash(&fs->chars, codepoint);
	if (fc != NULL)
		return fc;

	fc = PUSH_ITEM(&Arena, font_character);
	fc->fs = fs;

	GetCodePointMetrics(fs, codepoint, fc);

	sub_rectangle *rect = AddToAtlas(&fs->atlas[fs->current_atlas], fc, fc->x1 - fc->x0, fc->y1 - fc->y0);

	if (!rect)
	{
		AddNewAtlasAndTextureToFontSizeRendering(fs);
		DBG("Font '%s' spread across %d texture(s)", f->name, fs->current_atlas + 1);
		rect = AddToAtlas(&fs->atlas[fs->current_atlas], fc, fc->x1 - fc->x0, fc->y1 - fc->y0);
	}

	fc->atlas_index = fs->current_atlas;
	fs->atlas_tex_updated[fs->current_atlas] = true;
	fc->tex_rect = rect;
	ComputeCharacterUV(fc);

	AddToHash(&fs->chars, fc, codepoint);

	//Draw the glyph
	transient_arena ta = GetTransientArena(&Arena);

	//Create a small canvas that will work for the glyph
	uint8_t *texture_data = PUSH_ARRAY(&Arena, uint8_t, rect->w*rect->h);

	//render out the glyph
	stbtt_MakeCodepointBitmap(&f->info, texture_data, rect->w, rect->h, rect->w, fs->scale,fs->scale,codepoint);

	//Draw to texture
	for (int y = rect->y; y < (rect->y + rect->h); y++)
	{
		uint32_t *start = (uint32_t *)(fs->rgba_tex_data[fs->current_atlas] + y*fs->atlas[fs->current_atlas].w*4 + rect->x*4);
		uint8_t *data = texture_data + (y - rect->y)*rect->w;
		for (int x = 0; x < rect->w; x++)
		{
			uint32_t color = MAKE_COLOR(255, 255, 255, *data);
			*start = color;
			start++;
			data++;
		}
	}

	FreeTransientArena(ta);

	//Re-up the tex 2 Op'GL
	if (render_immediately)
	{
		glBindTexture(GL_TEXTURE_2D, fs->tex[fs->current_atlas]->gl_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fs->tex[fs->current_atlas]->width, fs->tex[fs->current_atlas]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fs->rgba_tex_data[fs->current_atlas]);
		fs->atlas_tex_updated[fs->current_atlas] = false;
	}

	return fc;
}


void RenderSize(font *f, int pixel_size, char *characters_to_render)
{
	//font_size is a struct that represents a single size of a single font, and holds a texture
	//for all the characters that were rendered for that font.
	font_size *fs = PUSH_ITEM(&Arena, font_size);
	*fs = {};
	fs->f = f;
	fs->size = pixel_size;
	fs->scale = stbtt_ScaleForPixelHeight(&f->info, pixel_size);

	int ascent;
	stbtt_GetFontVMetrics(&f->info, &ascent,0,0);
	fs->baseline = (int) (ascent*fs->scale);

	fs->current_atlas = -1;
	InitHash(&fs->chars, 64);


	int c;
	void *str_iter = characters_to_render;
	str_iter = utf8codepoint(str_iter, &c);

	while(c)
	{
		AddCharacterToFontSize(fs, c, false);
		str_iter = utf8codepoint(str_iter, &c);
	}

	for (int i = 0; i < fs->current_atlas + 1; i++)
	{
		glBindTexture(GL_TEXTURE_2D, fs->tex[i]->gl_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fs->tex[i]->width, fs->tex[i]->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fs->rgba_tex_data[i]);
		fs->atlas_tex_updated[i] = false;
	}

	//Now add the font size to the font
	AddToHash(&f->sizes, fs, pixel_size);
}


float GetAdvanceForCharacter(font *f, font_size *fs, font_character *fc, int codepoint, int next_codepoint)
{
	float result = 0;
	if (fc == NULL)
		return result;

	result = fc->move_cursor_x;
	if (next_codepoint)
		result += fs->scale*stbtt_GetCodepointKernAdvance(&f->info, codepoint, next_codepoint);

	return result;
}
