enum text_alignment
{
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_CENTER,
};

struct font
{
	enum Language
	{
		ENGLISH,
		JAPANESE,
	};
	char name[64];
	char filename[1024];
	Language language;
	uint8_t *font_file_data;
	size_t font_file_length;
	stbtt_fontinfo info;
	generic_hash sizes;
};

struct font_size
{
	font *f;
	int size;
	int baseline;
	float scale;
	generic_hash chars;

	int current_atlas; //can still add characters to this atlas
	texture *tex[4];
	uint8_t *rgba_tex_data[4];
	rectangle_atlas atlas[4];
	bool atlas_tex_updated[4];
};

struct font_character
{
	int codepoint;
	int advance;
	int lsb;
	int x0, x1, y0, y1;
	float move_cursor_x;
	sub_rectangle *tex_rect;
	float u0, u1, v0, v1;
	int atlas_index;
	font_size *fs;
};

void InitFont(font *f, const char *name, void *fileData, size_t fontLength);
font_character *AddCharacterToFontSize(font_size *fs, int codepoint, bool render_immediately = false);
void RenderSize(font *f, int pixel_size, char *characters_to_render);
float GetAdvanceForCharacter(font *f, font_size *fs, font_character *fc, int codepoint, int next_codepoint);
