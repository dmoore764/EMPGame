void AddAtlasToSpritePack(sprite_pack *pack)
{
	int i = pack->num_atlases++;
	pack->current_atlas = i;
	sprite_atlas *atlas = &pack->atlas[i];
	rectangle_atlas *rect_atlas = &atlas->atlas;
	InitAtlas(rect_atlas);
	rect_atlas->packed = true; //Put into the atlas immediately
	rect_atlas->outer_border = 0;
	rect_atlas->item_border = 2;
	rect_atlas->w = 2048;
	rect_atlas->h = 2048;

	atlas->rgba_tex_data = PUSH_ARRAY(&Arena, uint8_t, rect_atlas->w*4*rect_atlas->h);
	memset(atlas->rgba_tex_data, '\0', rect_atlas->w*4*rect_atlas->h);

	texture *newtex = PUSH_ITEM(&Arena, texture);
	newtex->width = rect_atlas->w;
	newtex->height = rect_atlas->h;
	CreateTexture(newtex, NULL, true, true);
	sprintf(newtex->name, "sprite pack (%d)", i);
	AddToHash(&Textures, newtex, newtex->name);
	atlas->tex = newtex;
}

game_sprite *AddSpriteToAvailableAtlas(sprite_pack *pack, dir_file *file, bool render_immediately)
{
	if (pack->num_atlases == 0)
	{
		AddAtlasToSpritePack(pack);
	}

	game_sprite *s = (game_sprite *)GetFromHash(&Sprites, file->basename);
	if (s == NULL)
	{
		s = PUSH_ITEM(&Arena, game_sprite);
		AddToHash(&Sprites, s, file->basename);
	}


	char src_file[KILOBYTES(1)];
	sprintf(src_file, "%s%s", file->folder_path, file->name);
	int n, w, h;
	uint8_t *image_data = stbi_load(src_file, &w, &h, &n, 0);

	sub_rectangle *rect = AddToAtlas(&pack->atlas[pack->current_atlas].atlas, s, w, h);
	if (!rect)
	{
		AddAtlasToSpritePack(pack);
		rect = AddToAtlas(&pack->atlas[pack->current_atlas].atlas, s, w, h);
	}

	s->atlas_index = pack->current_atlas;
	sprite_atlas *atlas = &pack->atlas[pack->current_atlas];
	rectangle_atlas *rect_atlas = &atlas->atlas;
	assert(rect); //Still couldn't find a space for this texture (too big?)
	s->tex_rect = rect;

	float one_over_tex_width = 1 / (float) pack->atlas[pack->current_atlas].atlas.w;
	float one_over_tex_height = 1 / (float) pack->atlas[pack->current_atlas].atlas.h;
	s->u0 = s->tex_rect->x * one_over_tex_width;
	s->u1 = (s->tex_rect->x + s->tex_rect->w) * one_over_tex_width;
	s->v0 = s->tex_rect->y * one_over_tex_height;
	s->v1 = (s->tex_rect->y + s->tex_rect->h) * one_over_tex_height;

	//TODO: render sprite to atlas rgba_tex_data
	
	for (int y = rect->y; y < (rect->y + rect->h); y++)
	{
		uint32_t *start = (uint32_t *)(atlas->rgba_tex_data + y*rect_atlas->w*4 + rect->x*4);
		uint32_t *data = (uint32_t *)(image_data + (y - rect->y)*rect->w*4);
		for (int x = 0; x < rect->w; x++)
		{
			*start = *data;
			start++;
			data++;
		}
	}

	stbi_image_free(image_data);

	if (render_immediately)
	{
		glBindTexture(GL_TEXTURE_2D, atlas->tex->gl_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->tex->width, atlas->tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->rgba_tex_data);
	}

	return s;
}


game_tile_sprites *AddTileSpritesToAvailableAtlas(sprite_pack *pack, dir_file *file, bool render_immediately)
{
	if (pack->num_atlases == 0)
	{
		AddAtlasToSpritePack(pack);
	}

	game_tile_sprites *s = (game_tile_sprites *)GetFromHash(&TileSprites, file->basename);
	if (s == NULL)
	{
		s = PUSH_ITEM(&Arena, game_tile_sprites);
		s->numTiles = 0;
		AddToHash(&TileSprites, s, file->basename);
	}

	char src_file[KILOBYTES(1)];
	sprintf(src_file, "%s%s", file->folder_path, file->name);
	int n, w, h;
	uint8_t *image_data = stbi_load(src_file, &w, &h, &n, 0);
	s->atlas_index = pack->current_atlas;
	sprite_atlas *atlas = &pack->atlas[pack->current_atlas];

	for (int y = 0; y < (h >> 3); y++)
	{
		for (int x = 0; x < (w >> 3); x++)
		{
			game_tile *t = &s->tiles[s->numTiles++];

			sub_rectangle *rect = AddToAtlas(&pack->atlas[pack->current_atlas].atlas, t, 14, 14);
			assert(rect); //need to load tiles in such a way that they all fit into the same atlas

			rectangle_atlas *rect_atlas = &atlas->atlas;
			t->tex_rect = rect;

			double one_over_tex_width = 1 / (double) pack->atlas[pack->current_atlas].atlas.w;
			double one_over_tex_height = 1 / (double) pack->atlas[pack->current_atlas].atlas.h;

			t->u0 = (float)((t->tex_rect->x + 3) * one_over_tex_width);
			t->u1 = (float)((t->tex_rect->x + 3 + 8) * one_over_tex_width);
			t->v0 = (float)((t->tex_rect->y + 3 + 8) * one_over_tex_height);
			t->v1 = (float)((t->tex_rect->y + 3) * one_over_tex_height);

			int imageY = y << 3;
			int imageX = x << 3;
			for (int y = rect->y; y < (rect->y + rect->h); y++)
			{
				uint32_t *start = (uint32_t *)(atlas->rgba_tex_data + y*rect_atlas->w*4 + rect->x*4);
				int currentY = y - rect->y;
				int yOffset = currentY < 3 ? 0 : (currentY > 10 ? 7 : currentY - 3);
				uint32_t *data = (uint32_t *)(image_data + (imageY + yOffset)*w*4 + imageX*4);
				for (int x = 0; x < rect->w; x++)
				{
					*start = *data;
					start++;
					if (x != 0 && x != 1 && x != 2 && x != 10 && x != 11 && x != 12)
						data++;
				}
			}
		}
	}

	stbi_image_free(image_data);

	if (render_immediately)
	{
		glBindTexture(GL_TEXTURE_2D, atlas->tex->gl_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->tex->width, atlas->tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->rgba_tex_data);
	}

	return s;
}
