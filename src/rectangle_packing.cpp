void InitAtlas(rectangle_atlas *atlas)
{
	atlas->packed = false;
	atlas->w = 0;
	atlas->h = 0;
	atlas->outer_border = 0;
	atlas->item_border = 0;
	atlas->num_rects = 0;
}

bool FindPlaceInAtlas(rectangle_atlas *atlas, int rect_index)
{
	bool placed_object = false;
	int w = atlas->rects[rect_index].w;
	int h = atlas->rects[rect_index].h;

	for (int y = atlas->outer_border; y + h < atlas->h - atlas->outer_border; y++)
	{
		for (int x = atlas->outer_border; x + w < atlas->w - atlas->outer_border; )
		{
			bool found_location = true;
			for (int i = 0; i < rect_index; i++)
			{
				sub_rectangle *test = &atlas->rects[i];
				//test->x and test->y have already been shifted by item_border
				//whereas x and y are the top left coordinates before the item_border
				if (x < test->x + test->w + atlas->item_border &&
						x + w + (atlas->item_border * 2) > test->x - atlas->item_border &&
						y < test->y + test->h + atlas->item_border &&
						y + h  + (atlas->item_border * 2) > test->y - atlas->item_border)
				{
					//collision, move on
					x = test->x + test->w + atlas->item_border;
					found_location = false;
					break;
				}
			}
			if (found_location)
			{
				atlas->rects[rect_index].x = x + atlas->item_border;
				atlas->rects[rect_index].y = y + atlas->item_border;
				placed_object = true;
				break;
			}
		}
		if (placed_object)
			break;
	}
	if (!placed_object)
	{
		return false;
	}

	return true;

}

sub_rectangle *AddToAtlas(rectangle_atlas *atlas, void *object, int w, int h)
{
	if (atlas->num_rects + 1 == ArrayCount(atlas->rects))
		return NULL;

	sub_rectangle *rect = &atlas->rects[atlas->num_rects++];
	rect->object = object;
	rect->w = w;
	rect->h = h;

	if (atlas->packed)
	{
		if (!FindPlaceInAtlas(atlas, atlas->num_rects-1))
		{
			atlas->num_rects--;
			return NULL;
		}
		else
			return rect;
	}
	else
	{
		//Not packed yet
		rect->x = -1;
		rect->y = -1;
		return NULL;
	}
}

bool PlaceRectanglesInAtlas(rectangle_atlas *atlas)
{
	for (int index = 0; index < atlas->num_rects; index++)
	{
		if (!FindPlaceInAtlas(atlas, index))
			return false;
	}
	return true;
}

void PackAtlas(rectangle_atlas *atlas)
{
	assert(!atlas->packed);

	//sort rectangles by area
	for (int i = 0; i < atlas->num_rects - 1; i++)
	{
		for (int j = 0; j < atlas->num_rects - 1 - i; j++)
		{
			int area1 = atlas->rects[j].w * atlas->rects[j].h;
			int area2 = atlas->rects[j + 1].w * atlas->rects[j + 1].h;
			if (area1 < area2) //move the smallest to the end
			{
				//swap
				sub_rectangle temp = atlas->rects[j + 1];
				atlas->rects[j + 1] = atlas->rects[j];
				atlas->rects[j] = temp;
			}
		}
	}

	int width_exp = 6; //2^6 = 64
	int height_exp = 6;
	atlas->w = 1 << width_exp;
	atlas->h = 1 << height_exp;
	bool packed = PlaceRectanglesInAtlas(atlas);
	while (!packed)
	{
		//try increasing horizontal dimension
		width_exp++;
		atlas->w = 2 << width_exp;
		packed = PlaceRectanglesInAtlas(atlas);
		if (packed)
			break;

		//try increasing vertical dimension instead
		height_exp++;
		atlas->h = 2 << height_exp;
		packed = PlaceRectanglesInAtlas(atlas);
	}

	atlas->packed = true;
}

void FillRectangleInTextureData(uint8_t *texdata, size_t tex_pitch, uint32_t color, int x, int y, int w, int h)
{
	//reverse for endianness????
	uint8_t r = (color & 0xff000000) >> 24;
	uint8_t g = (color & 0xff0000) >> 16;
	uint8_t b = (color & 0xff00) >> 8;
	uint8_t a = color & 0xff;

	uint32_t c = MAKE_COLOR(a, b, g, r);
	for (int row = 0; row < h; row++)
	{
		uint32_t *rowstart = (uint32_t *)(texdata + (((y + row) * tex_pitch) + (4 * x)));
		for (int col = 0; col < w; col++)
		{
			*rowstart = c;
			rowstart++;
		}
	}
}

void TestAtlasPacking()
{
	const char *object0 = "obj0";
	float w0 = 100;
	float h0 = 88;

	const char *object1 = "obj1";
	float w1 = 24;
	float h1 = 40;

	const char *object2 = "obj2";
	float w2 = 94;
	float h2 = 20;

	const char *object3 = "obj3";
	float w3 = 130;
	float h3 = 30;

	const char *object4 = "obj4";
	float w4 = 17;
	float h4 = 56;

	const char *object5 = "obj5";
	float w5 = 29;
	float h5 = 15;

	const char *object6 = "obj6";
	float w6 = 64;
	float h6 = 64;

	rectangle_atlas atlas = {};
	InitAtlas(&atlas);

	atlas.outer_border = 2;
	atlas.item_border = 2;

	AddToAtlas(&atlas, (void *)object0, w0, h0);
	AddToAtlas(&atlas, (void *)object1, w1, h1);
	AddToAtlas(&atlas, (void *)object2, w2, h2);
	AddToAtlas(&atlas, (void *)object3, w3, h3);
	AddToAtlas(&atlas, (void *)object4, w4, h4);
	AddToAtlas(&atlas, (void *)object5, w5, h5);
	AddToAtlas(&atlas, (void *)object6, w6, h6);

	PackAtlas(&atlas);

	/*
	printf("Atlas size: %d, %d\n", atlas.w, atlas.h);
	for (int i = 0; i < atlas.num_rects; i++)
	{
		sub_rectangle *rect = &atlas.rects[i];
		printf("Rect: '%s' at (%d, %d)\n", (const char *)rect->object, rect->x, rect->y);
	}
	*/

	transient_arena ta = GetTransientArena(&Arena);

	uint8_t *texture_data = PUSH_ARRAY(&Arena, uint8_t, 4*atlas.w*atlas.h);
	FillRectangleInTextureData(texture_data, atlas.w*4, COL_WHITE, 0, 0, atlas.w, atlas.h);

	for (int i = 0; i < atlas.num_rects; i++)
	{
		sub_rectangle *rect = &atlas.rects[i];
		FillRectangleInTextureData(texture_data, atlas.w*4, MAKE_COLOR(rand() % 256, rand() % 256, rand() % 256, 255), rect->x, rect->y, rect->w, rect->h);
	}

	texture t = {};
	t.width = atlas.w;
	t.height = atlas.h;
	CreateTexture(&t, texture_data, true, true);

	FreeTransientArena(ta);

	texture *newtex = PUSH_ITEM(&Arena, texture);
	*newtex = t;
	AddToHash(&Textures, newtex, "packed atlas");
}
