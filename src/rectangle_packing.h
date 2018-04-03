struct sub_rectangle
{
	int w;
	int h;
	int x;
	int y;

	void *object; //pointer back to the object that needs this data
};

struct rectangle_atlas
{
	bool packed;

	int w;
	int h;

	int outer_border;
	int item_border;

	int num_rects;
	sub_rectangle rects[500];
};

void InitAtlas(rectangle_atlas *atlas);
sub_rectangle *AddToAtlas(rectangle_atlas *atlas, void *object, int w, int h);
bool PlaceRectanglesInAtlas(rectangle_atlas *atlas);
void PackAtlas(rectangle_atlas *atlas);
void FillRectangleInTextureData(uint8_t *texdata, size_t tex_pitch, uint32_t color, int x, int y, int w, int h);
void TestAtlasPacking();
