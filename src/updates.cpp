global_variable char DebugMessage[128];
global_variable int DebugMessageCounter;

enum save_and_load_options
{
	SAVE,
	LOAD,
};
global_variable save_and_load_options SaveOrLoad;
global_variable json_value *LoadData;
global_variable char *SaveData;

global_variable bool LevelLoaderInitialized;
global_variable int OnLevel = 0;
global_variable int LoadedLevel = 0;

inline v2 ToScreen(ivec2 p)
{
	v2 result = {(float)(p.x - Camera.pos.x), (float)(p.y - Camera.pos.y)};
	return result;
}

inline v2 ToScreen(int x, int y)
{
	v2 result = {(float)(x - Camera.pos.x), (float)(y - Camera.pos.y)};
	return result;
}

inline ivec2 FromScreen(vec2 p)
{
	ivec2 result = {Camera.pos.x + (int)p.x, Camera.pos.y + (int)p.y};
	return result;
}

inline uint8_t GetCounter(uint32_t counter, int byteNum)
{
	uint8_t result = (uint8_t)((counter >> (byteNum << 3)) & 0xFF);
	return result;
}

inline void ClearCounter(uint32_t *counter, int byteNum)
{
	uint32_t clearMask = 0xFFFFFFFF ^ (0xFF << (byteNum << 3));
	*counter &= clearMask;
}

inline uint8_t SetCounter(uint32_t *counter, int byteNum, uint8_t val)
{
	ClearCounter(counter, byteNum);
	uint32_t v = (uint32_t)val << (byteNum << 3);
	*counter |= v;
	return *counter;
}

inline uint8_t IncrementCounter(uint32_t *counter, int byteNum)
{
	uint8_t val = GetCounter(*counter, byteNum);
	val += 1;
	SetCounter(counter, byteNum, val);
	return val;
}

inline uint8_t IncrementCounterNoLoop(uint32_t *counter, int byteNum)
{
	uint8_t val = GetCounter(*counter, byteNum);
	if (val < 255)
		val += 1;
	SetCounter(counter, byteNum, val);
	return val;
}

inline uint8_t IncrementCounterByAmountNoLoop(uint32_t *counter, int byteNum, int amount)
{
	uint8_t val = GetCounter(*counter, byteNum);
	if (val + amount <= 255)
		val += amount;
	else
		val = 255;
	SetCounter(counter, byteNum, val);
	return val;
}

inline uint8_t DecrementCounter(uint32_t *counter, int byteNum)
{
	uint8_t val = GetCounter(*counter, byteNum);
	if (val > 0)
		val -= 1;
	SetCounter(counter, byteNum, val);
	return val;
}

inline uint8_t DecrementCounterTo(uint32_t *counter, int byteNum, int to)
{
	uint8_t val = GetCounter(*counter, byteNum);
	if (val > to)
		val -= 1;
	else if (val < to)
		val = to;
	SetCounter(counter, byteNum, val);
	return val;
}

UPDATE_FUNCTION(EmptyUpdate) {}

#define GET_CAMERA_FOCUS(Name) ivec2 Name()
typedef GET_CAMERA_FOCUS(get_camera_focus);

global_variable get_camera_focus *FocusFunction;
global_variable ivec2 GameCamFocus;


#define CAM_UPDATE(Name) void Name()
typedef CAM_UPDATE(cam_update);

global_variable cam_update *CameraUpdateFunction;
global_variable ivec2 GameCamera;

CAM_UPDATE(CameraFreeze) {}

CAM_UPDATE(CameraLerpFunction)
{
	int moveX = (int)(GameCamFocus.x - GameCamera.x) >> 5;
	int moveY = (int)(GameCamFocus.y - GameCamera.y) >> 5;

	GameCamera.x += moveX;
	GameCamera.y += moveY;
	Camera.pos.x = GameCamera.x;
	Camera.pos.y = GameCamera.y;
}


struct irect
{
	ivec2 BL;
	ivec2 UR;
};

global_variable irect CameraRestraintAreas[20];
global_variable int NumCameraRestraintAreas;
global_variable ivec2 InitialCameraPos;

GET_CAMERA_FOCUS(PlayerFocus)
{
	ivec2 result = GameCamFocus;

	for (int i = 0; i < NumGameObjects; i++)
	{
		if (GameComponents.type[i] == OBJ_PLAYER)
		{
			auto tx = &GameComponents.transform[i];
			auto phy = &GameComponents.physics[i];
			result.x = tx->pos.x + Sign(tx->scale.x) * (10 << 9) + (phy->vel.x << 3);
			result.y = tx->pos.y + (35 << 9) + (phy->vel.y < 0 ? (phy->vel.y << 5) : 0);
			break;
		}
	}

	if (NumCameraRestraintAreas)
	{
		//first find the minimum and maximum x position of all the restraints
		int64_t min = INT_MAX;
		int64_t max = INT_MIN;
		for (int i = 0; i < NumCameraRestraintAreas; i++)
		{
			irect *area = &CameraRestraintAreas[i];
			if (area->BL.x < min)
				min = area->BL.x;
			if (area->UR.x > max)
				max = area->UR.x;
		}
		ClampToRange(min, &result.x, max);

		//based on the x position, use the camera restraints to set y
		min = INT_MAX;
		max = INT_MIN;
		for (int i = 0; i < NumCameraRestraintAreas; i++)
		{
			irect *area = &CameraRestraintAreas[i];
			if (result.x <= area->UR.x && result.x >= area->BL.x)
			{
				if (area->BL.y < min)
					min = area->BL.y;
				if (area->UR.y > max)
					max = area->UR.y;
			}
		}
		ClampToRange(min, &result.y, max);
	}

	return result;
}

GET_CAMERA_FOCUS(FreelyMoveableFocus)
{
	if (KeyboardDown[KB_LEFT])
		GameCamFocus.x -= 0x500;
	else if (KeyboardDown[KB_RIGHT])
		GameCamFocus.x += 0x500;

	if (KeyboardDown[KB_DOWN])
		GameCamFocus.y -= 0x500;
	else if (KeyboardDown[KB_UP])
		GameCamFocus.y += 0x500;
	return GameCamFocus;
}

enum tile_types
{
	EMPTY				= 0,
	SQUARE,
	SMALL_SLOPE_LEFT1,
	SMALL_SLOPE_LEFT2,
	LARGE_SLOPE_LEFT,
	SMALL_SLOPE_RIGHT1,
	SMALL_SLOPE_RIGHT2,
	LARGE_SLOPE_RIGHT,
	PLATFORM,
	LADDER,
	LADDER_TOP,
	ONE_WAY,
};

enum player_flags
{
	CONTROL_LOCKED	= 0x1,
	ON_GROUND		= 0x2,
	HOLDING_JUMP	= 0x4,
	ON_SLOPE		= 0x8,
	LEFT_SLOPE		= 0x10,
	SLIDING			= 0x20,
	SKIDDING		= 0x40, //sliding on flat ground
	SLIDE_LEFT		= 0x80,
	ON_LADDER		= 0x100,
	ON_WALL			= 0x200,
	USED_WALL_JUMP	= 0x400,
	WALL_ON_LEFT	= 0x800,
	WALL_ON_RIGHT	= 0x1000,
	WALL_JUMPING	= 0x2000,
};

enum player_counters
{
	PLYR_SINCE_LEFT_GROUND		= 0,
	PLYR_WALL_TIME				= 1,
	PLYR_ITEM_CARRIED			= 2,
	PLYR_TIME_SINCE_JUMP		= 3,
};

enum player_counters2
{
	PLYR_CREATE_DUST			= 0,
	PLYR_INVICIBLE				= 1,
	PLYR_KNOCKBACK				= 2,
};

inline void DrawPoint(uint32_t color, uint32_t x, uint32_t y)
{
	v2 p = ToScreen(x, y);
	DrawQuad(color, p, {p.x + 0x200, p.y}, {p.x + 0x200, p.y + 0x200}, {p.x, p.y + 0x200}); 
}

UPDATE_FUNCTION(PlayerDraw)
{
	auto sprite = GO(sprite);
	auto tx = GO(transform);
	auto counters = GO(counters);

	SetShader("basic_tex_color");
	
	game_sprite *s = (game_sprite *)GetFromHash(&Sprites, sprite->name);
    DrawSprite(s, COL_WHITE, ToScreen(tx->pos.x, tx->pos.y), tx->rot, {tx->scale.x * 0x200, tx->scale.y * 0x200});

	uint8_t carrying = GetCounter(counters->counters, PLYR_ITEM_CARRIED);
	switch (carrying & 0xf0)
	{
		case 0x10: //crystal
		{
			DrawSprite("Crystal", COL_WHITE, ToScreen(tx->pos + ivec2(0,12<<9)), 0, {0x200,0x200});
		} break;
	}

	/*
	poi *p = GetPOI(s, "hand");
	if (p)
	{
		ivec2 attachPoint = p->location;
		char *weapon = "Wand";
		if (KeyDown[KEY_Action2])
			weapon = "WandInUse";
		DrawSprite(weapon, COL_WHITE, ToScreen(tx->pos.x + ((int)(attachPoint.x * Sign(tx->scale.x)) << 9), tx->pos.y + (attachPoint.y << 9)), tx->rot, {tx->scale.x * 0x200, tx->scale.y * 0x200});
	}
	*/

	/*
	SetShader("basic");
	//centerfoot, leftfoot, rightfoot
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x, tx->pos.y - 0x200);
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x - (3 << 9), tx->pos.y - (1 << 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x + (3 << 9), tx->pos.y - (1 << 9));

	//head
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x - (4 << 9), tx->pos.y + (15 << 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x + (4 << 9), tx->pos.y + (15 << 9));

	//left/right side
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x - (6 << 9), tx->pos.y + (3 << 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x + (6 << 9), tx->pos.y + (3 << 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x - (6 << 9), tx->pos.y + (11<< 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x + (6 << 9), tx->pos.y + (11<< 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x - (6 << 9), tx->pos.y + (15<< 9));
	DrawPoint(MAKE_COLOR(255,100,0,255), tx->pos.x + (6 << 9), tx->pos.y + (15<< 9));
	*/
}

enum collision_bits
{
	CENTER_FOOT			= 0x1,
	LEFT_FOOT			= 0x2,
	RIGHT_FOOT			= 0x4,
	HEAD				= 0x8,
	LEFT_SIDE_LOWER		= 0x10,
	RIGHT_SIDE_LOWER	= 0x20,
	LEFT_SIDE_MID		= 0x40,
	RIGHT_SIDE_MID		= 0x80,
};

enum tile_sets
{
	TS_DIRT			= 0,

	TS_COUNT,
};

struct tile_set_tile_types
{
	uint8_t types[255];
};

global_variable tile_set_tile_types TileSetTypes[TS_COUNT];

struct tile_paint
{
	char *name;
	int pitch;
	int rows;
};

global_variable tile_paint TilePaint[TS_COUNT];

struct tile_brush_part
{
	int xOffset;
	int yOffset;
	uint8_t tileNum;
	bool front;
};

struct tile_brush
{
	tile_brush_part parts[10];
	int numParts;
};

global_variable tile_brush Brushes[TS_COUNT][100];

struct tile_paint_brush
{
	int fromBrushes[10];
	int numBrushes;
};

global_variable tile_paint_brush TilePaintBrush[TS_COUNT][20];
global_variable int NumTilePaintBrushes[TS_COUNT];

void InitTiles()
{
	TileSetTypes[TS_DIRT] = {
		EMPTY, EMPTY, SMALL_SLOPE_RIGHT1, SMALL_SLOPE_LEFT1, EMPTY, EMPTY, EMPTY, EMPTY,
		SQUARE, SQUARE, SQUARE, SQUARE, ONE_WAY, SMALL_SLOPE_RIGHT2, SMALL_SLOPE_LEFT2, EMPTY,
		SQUARE, SQUARE, SQUARE, SQUARE, SQUARE, SQUARE, SQUARE, SQUARE,
		LADDER,
	};

	TilePaint[TS_DIRT] = {
		"TestDirtTiles",
		8,
		8
	};

	Brushes[TS_DIRT][0] = {
		{
			{ 0,  1,  0, true },
			{ 0,  0,  8, true },
		},
		2
	};
	Brushes[TS_DIRT][1] = {
		{
			{ 0,  1,  1, true },
			{ 0,  0,  9, true },
		},
		2
	};
	TilePaintBrush[TS_DIRT][0] = {
		{ 0, 1 },
		2
	};

	Brushes[TS_DIRT][2] = {
		{
			{ 0,  1,  2, true },
			{ 0,  0,  10, true },
		},
		2
	};
	TilePaintBrush[TS_DIRT][1] = {
		{ 2 },
		1
	};

	Brushes[TS_DIRT][3] = {
		{
			{ 0,  1,  3, true },
			{ 0,  0,  11, true },
		},
		2
	};
	TilePaintBrush[TS_DIRT][2] = {
		{ 3 },
		1
	};

	Brushes[TS_DIRT][4] = {
		{
			{ 0,  1,  4, true },
			{ 0,  0,  12, true },
		},
		2
	};
	TilePaintBrush[TS_DIRT][3] = {
		{ 4 },
		1
	};

	Brushes[TS_DIRT][5] = {
		{
			{ 0,  2,  5, true },
			{ 0,  1,  13, true },
			{ 0,  0,  21, true },
		},
		3
	};
	TilePaintBrush[TS_DIRT][4] = {
		{ 5 },
		1
	};

	Brushes[TS_DIRT][6] = {
		{
			{ 0,  2,  6, true },
			{ 0,  1,  14, true },
			{ 0,  0,  22, true },
		},
		3
	};
	TilePaintBrush[TS_DIRT][5] = {
		{ 6 },
		1
	};

	Brushes[TS_DIRT][7] = { {{ 0,  0,  17, true }}, 1 };
	Brushes[TS_DIRT][8] = { {{ 0,  0,  18, true }}, 1 };
	Brushes[TS_DIRT][9] = { {{ 0,  0,  19, true }}, 1 };
	Brushes[TS_DIRT][10] = { {{ 0,  0,  20, true }}, 1 };
	Brushes[TS_DIRT][11] = { {{ 0,  0,  21, true }}, 1 };
	Brushes[TS_DIRT][12] = { {{ 0,  0,  22, true }}, 1 };
	TilePaintBrush[TS_DIRT][6] = {
		{ 7, 8, 9, 10, 11, 12 },
		6
	};

	Brushes[TS_DIRT][13] = { {{ 0,  0,  16, true }}, 1 };
	TilePaintBrush[TS_DIRT][7] = {
		{ 13 },
		1
	};

	Brushes[TS_DIRT][14] = { {{ 0,  0,  63, true }}, 1 };
	TilePaintBrush[TS_DIRT][8] = {
		{ 14 },
		1
	};

	Brushes[TS_DIRT][15] = { {{ 0,  0,  24, false }}, 1 };
	TilePaintBrush[TS_DIRT][9] = {
		{ 15 },
		1
	};


	NumTilePaintBrushes[TS_DIRT] = 10;
}

enum tile_flags
{
	TILE_IN_USE			= 0x1,
	MODIFIED_TILE_TYPE	= 0x2,
	HAS_FRONT			= 0x4,
	HAS_BACK			= 0x8,
};

//tileSet and tileNum both are needed to specify the graphic

uint32_t NewTile(uint8_t tileNumF, uint8_t tileNumB, uint8_t tileSet, uint8_t tileFlags)
{
	return MAKE_COLOR(tileNumB,tileNumF,tileSet,tileFlags);
}

inline uint8_t TileFlags(uint32_t tile)
{
	return ((tile & 0xff000000) >> 24);
}

inline uint8_t TileSet(uint32_t tile)
{
	return ((tile & 0xff0000) >> 16);
}

inline uint8_t TileNumF(uint32_t tile)
{
	return ((tile & 0xff00) >> 8);
}

inline uint8_t TileNumB(uint32_t tile)
{
	return (tile & 0xff);
}

inline uint8_t TileAtPoint(ivec2 p, int y)
{
	p.x >>= 12;
	p.y >>= 12;
	if (p.x >= 0 && p.y >= 0 && p.x < MAP_W && p.y < MAP_H)
	{
		uint8_t t = TileMapI[p.y*MAP_W + p.x];
		if (t == ONE_WAY)
		{
			int py = (p.y + 1) << 12;
			t &= ~0xff;
			if (y >= py)
				t |= SQUARE;
		}
		else if (t == LADDER_TOP)
		{
			int py = (p.y + 1) << 12;
			t &= ~0xff;
			if (y >= py)
				t |= SQUARE;
			else
				t |= LADDER;
		}

		return t;
	}
	return 0;
}

inline uint8_t TileAtPoint(ivec2 p)
{
	p.x >>= 12;
	p.y >>= 12;
	if (p.x >= 0 && p.y >= 0 && p.x < MAP_W && p.y < MAP_H)
	{
		uint8_t t = TileMapI[p.y*MAP_W + p.x];
		return t;
	}
	return 0;
}

int GetGroundYForTileAndPos(uint32_t tileX, uint32_t tileY, uint8_t tile, int posX)
{
	int result = tileY + (8 << 9);
	switch (tile)
	{
		case SMALL_SLOPE_LEFT1:
		{
			tileY += (4 << 9);
			int slope = -4;
			int offsetFromLeft = (posX - tileX);
			int shiftDown = (offsetFromLeft * slope) >> 3;
			result = tileY + shiftDown;
		} break;

		case SMALL_SLOPE_LEFT2:
		{
			tileY += (8 << 9);
			int slope = -4;
			int offsetFromLeft = (posX - tileX);
			int shiftDown = (offsetFromLeft * slope) >> 3;
			result = tileY + shiftDown;
		} break;

		case LARGE_SLOPE_LEFT:
		{
			tileY += (8 << 9);
			int slope = -8;
			int offsetFromLeft = (posX - tileX);
			int shiftDown = (offsetFromLeft * slope) >> 3;
			result = tileY + shiftDown;
		} break;

		case SMALL_SLOPE_RIGHT1:
		{
			int slope = 4;
			int offsetFromLeft = (posX - tileX);
			int shiftDown = (offsetFromLeft * slope) >> 3;
			result = tileY + shiftDown;
		} break;

		case SMALL_SLOPE_RIGHT2:
		{
			tileY += (4 << 9);
			int slope = 4;
			int offsetFromLeft = (posX - tileX);
			int shiftDown = (offsetFromLeft * slope) >> 3;
			result = tileY + shiftDown;
		} break;

		case LARGE_SLOPE_RIGHT:
		{
			int slope = 8;
			int offsetFromLeft = (posX - tileX);
			int shiftDown = (offsetFromLeft * slope) >> 3;
			result = tileY + shiftDown;
		} break;
	}

	return result;
}

bool CollidingWithTileAtPoint(uint32_t x, uint32_t y, uint32_t *newY)
{
	bool result = false;
	uint8_t t = TileAtPoint({x, y});
	switch (t)
	{
		case EMPTY: {} break;
		case SQUARE:
		{
			if (newY)
				*newY = ((y >> 12) + 1) << 12;
			result = true;
		} break;
		case SMALL_SLOPE_LEFT1:
		case SMALL_SLOPE_LEFT2:
		case LARGE_SLOPE_LEFT:
		case SMALL_SLOPE_RIGHT1:
		case SMALL_SLOPE_RIGHT2:
		case LARGE_SLOPE_RIGHT:
		{
			int slopeY = GetGroundYForTileAndPos((x >> 12) << 12, (y >> 12) << 12, t, x);
			if (y < slopeY)
			{
				if (newY)
					*newY = slopeY;
				result = true;
			}
		} break;
	}
	return result;
}

struct magic_bullet_trail
{
	ivec2 pos;
	uint32_t color;
	float timeLeft;
};

enum magic_bullet_counters
{
	MB_ROTATION			= 0,
	MB_SIN_AMP			= 1,
	MB_TIME_ALIVE		= 2,
	MB_NEXT_SMOKE		= 3
};



enum dialogue_modes
{
	DLG_MODE_ENTER,
	DLG_MODE_INPLACE,
	DLG_MODE_LINESHIFT,
	DLG_MODE_NLPAUSE,
};

enum dialogue_counters
{
	DLG_ALPHA			= 0,
	DLG_FRAMESTONEXTC	= 1,
};

struct dialogue_character
{
	ivec2 pos;
	int codepoint;
	uint32_t color;
	int displayTime; //frames before moving on to next character
	int lineNumber;
};

struct dialogue_box
{
	game_sprite *s; //The sprite for the actual box
	ivec2 startPos;
	font *f;
	font_size *fs;
	dialogue_character text[1024];
	int numCharacters; //This will be higher than drawTo when we are still printing out the dialogue
	int drawTo; //point in text to draw to (changes as letters are drawn to screen)
	int displayWidth; //size of area to allow characters before wrapping
	ivec2 cursorPos; //where the next character would display
	ivec2 lineSize; //for horizontal text, x will be 0
	ivec2 lineShift;
	int numLines;
	dialogue_modes mode;
};

global_variable dialogue_box DialogueBox;

void DBRemoveTopLine()
{
	int numToDelete = 0;
	for (int i = 0; i < DialogueBox.numCharacters; i++)
	{
		DialogueBox.text[i].lineNumber--;
		DialogueBox.text[i].pos -= DialogueBox.lineSize;
		if (DialogueBox.text[i].lineNumber == 0)
			numToDelete++;
	}

	//Remove top line
	for (int i = 0; i < DialogueBox.numCharacters - numToDelete; i++)
	{
		DialogueBox.text[i] = DialogueBox.text[i + numToDelete];
	}
	DialogueBox.numCharacters -= numToDelete;
	DialogueBox.drawTo -= numToDelete;
}

void DBAddLine(char *line)
{
	DialogueBox.drawTo = DialogueBox.numCharacters;
	//calculate dialog cursor pos
	int curLineNumber = 1;
	if (DialogueBox.numCharacters > 0)
	{
		if (DialogueBox.mode == DLG_MODE_LINESHIFT)
		{
			DialogueBox.lineShift = {0,0};
			DialogueBox.mode = DLG_MODE_INPLACE;
		}
		for (int i = 0; i < DialogueBox.numCharacters; i++)
		{
			if (DialogueBox.text[i].lineNumber > curLineNumber)
				curLineNumber = DialogueBox.text[i].lineNumber;
		}
		while (curLineNumber > DialogueBox.numLines)
		{
			DBRemoveTopLine();
			curLineNumber--;
		}

		DialogueBox.cursorPos = {(int)DialogueBox.startPos.x + (curLineNumber * DialogueBox.lineSize.x), (int)DialogueBox.startPos.y + (curLineNumber * DialogueBox.lineSize.y)};
		curLineNumber++;
	}
	else
		DialogueBox.cursorPos = DialogueBox.startPos;

	void *str_iter = line;
	int chars_to_draw = utf8len(line);
	for (int i = 0; i < chars_to_draw; i++)
	{
		int c;
		str_iter = utf8codepoint(str_iter, &c);
		font_character *fc = (font_character *)GetFromHash(&DialogueBox.fs->chars, c);
		if (fc == NULL)
			fc = AddCharacterToFontSize(DialogueBox.fs, c, false);

		//get next word length
		{
			int cPrev = c;
			void *wordLengthIter = str_iter;
			int length = 0;
			while (cPrev && cPrev != ' ')
			{
				int cNext;
				wordLengthIter = utf8codepoint(wordLengthIter, &cNext);
				length += (int)(GetAdvanceForCharacter(DialogueBox.f, DialogueBox.fs, fc, c, cNext) * 4.0) << 7;
				cPrev = cNext;
			}
			if (DialogueBox.cursorPos.x - DialogueBox.startPos.x + length > DialogueBox.displayWidth)
			{
				//move to next line
				DialogueBox.cursorPos = {DialogueBox.startPos.x + DialogueBox.lineSize.x, DialogueBox.cursorPos.y + DialogueBox.lineSize.y};
				curLineNumber++;
			}
		}

		dialogue_character *character = &DialogueBox.text[DialogueBox.numCharacters++];
		character->codepoint = c;
		character->color = COL_WHITE;
		character->displayTime = 2;
		character->pos = DialogueBox.cursorPos;
		character->lineNumber = curLineNumber;

		int cNext;
		utf8codepoint(str_iter, &cNext);
		DialogueBox.cursorPos.x += (int)(GetAdvanceForCharacter(DialogueBox.f, DialogueBox.fs, fc, c, cNext) * 4.0) << 7;

	}
}

vec2 CoordConversion(vec2 coord, glm::mat4 *start, glm::mat4 *end)
{
	vec2 result = {0,0};
	return result;
}

void DrawDialogBox(uint8_t alpha, ivec2 pos)
{
	DrawSprite(DialogueBox.s, WITH_A(COL_WHITE, alpha), {(float)(pos.x >> 9), (float)(pos.y >> 9)}, 0, {2,2});

	vec2 bl = CoordConversion(vec2(0, 8), &GameGuiVP, &GameWindowProj);
	vec2 ur = CoordConversion(vec2(2000, 8 + (40 << 1)), &GameGuiVP, &GameWindowProj);
	EnableScissor(bl.x, ur.x, bl.y, ur.y);

	Rend.currentFont = DialogueBox.f;
	Rend.currentFontSize = DialogueBox.fs;
	for (int atlas_index = 0; atlas_index < DialogueBox.fs->current_atlas + 1; atlas_index++)
	{
		bool needToShift = false;
		texture *t = DialogueBox.fs->tex[atlas_index];
		Rend.recentlySetTexture = t;
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_TEXTURE;
		cmd->tex = t;
		cmd->tex_slot = 0;
		for (int i = 0; i < DialogueBox.drawTo; i++)
		{
			dialogue_character *c = &DialogueBox.text[i];
			font_character *fc = (font_character *)GetFromHash(&Rend.currentFontSize->chars, c->codepoint);

			if (fc->atlas_index == atlas_index)
			{
				render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
				cmd->type = RC_ADD_VERTICES;
				cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
				cmd->num_vertices = 6;

				int shiftX = DialogueBox.lineShift.x;
				int shiftY = DialogueBox.lineShift.y;

				float px_l = ((c->pos.x + shiftX) >> 9) + (float)fc->x0;
				float px_r = ((c->pos.x + shiftX) >> 9) + (float)fc->x1;
				float py_t = ((c->pos.y + shiftY) >> 9) - (float)fc->y0;
				float py_b = ((c->pos.y + shiftY) >> 9) - (float)fc->y1;
				float u0 = fc->u0, u1 = fc->u1, v0 = fc->v0, v1 = fc->v1;
				uint32_t color = Rend.currentFontColor;

				PushVertex(&px_l, &py_t, &color, &u0, &v0);
				PushVertex(&px_r, &py_t, &color, &u1, &v0);
				PushVertex(&px_l, &py_b, &color, &u0, &v1);
				PushVertex(&px_l, &py_b, &color, &u0, &v1);
				PushVertex(&px_r, &py_t, &color, &u1, &v0);
				PushVertex(&px_r, &py_b, &color, &u1, &v1);
			}
		}
	}
	DisableScissor();
}


UPDATE_FUNCTION(DialogueUpdateAndDrawGameGui)
{
	auto tx = GO(transform);
	auto phy = GO(physics);
	auto counters = GO(counters); //0 - alpha, 1 - count before showing next character, 2 - maybe picture to show?
	local_persistent int nlPause = 0;
	
	switch (DialogueBox.mode)
	{
		case DLG_MODE_ENTER:
		{
			//swoop in the dialog box
			IncrementCounterByAmountNoLoop(&counters->counters, DLG_ALPHA, 15);
			if (tx->pos.y > (5 << 9))
			{
				tx->pos.y = (5 << 9);
				phy->vel.y = 0;
				SetCounter(&counters->counters, DLG_ALPHA, 255);
				DialogueBox.mode = DLG_MODE_INPLACE;
				SetCounter(&counters->counters, DLG_FRAMESTONEXTC, 0);
			}
		} break;

		case DLG_MODE_INPLACE:
		{
			int nextC = DialogueBox.drawTo;
			if (nextC < DialogueBox.numCharacters && DialogueBox.text[nextC].lineNumber > DialogueBox.numLines)
			{
				DialogueBox.mode = DLG_MODE_LINESHIFT;
			}
			else
			{
				//Advance cursor
				DecrementCounter(&counters->counters, DLG_FRAMESTONEXTC);
				while (GetCounter(counters->counters, DLG_FRAMESTONEXTC) == 0 && DialogueBox.drawTo < DialogueBox.numCharacters)
				{
					DialogueBox.drawTo++;
					SetCounter(&counters->counters, DLG_FRAMESTONEXTC, DialogueBox.text[DialogueBox.drawTo].displayTime);
				}
			}
		} break;

		case DLG_MODE_LINESHIFT:
		{
			bool shifted = false;
			if (DialogueBox.lineShift.x + DialogueBox.lineSize.x != 0)
			{
				shifted = true;
				DialogueBox.lineShift.x -= DialogueBox.lineSize.x / 14;
				if (Abs(DialogueBox.lineSize.x) - Abs(DialogueBox.lineShift.x) < 0x300)
					DialogueBox.lineShift.x = -DialogueBox.lineSize.x;
			}
			if (DialogueBox.lineShift.y + DialogueBox.lineSize.y != 0)
			{
				shifted = true;
				DialogueBox.lineShift.y -= DialogueBox.lineSize.y / 14; //14 frames to shift
				if (Abs(DialogueBox.lineSize.y) - Abs(DialogueBox.lineShift.y) < 0x300)
					DialogueBox.lineShift.y = -DialogueBox.lineSize.y;
			}
			if (!shifted)
			{
				DialogueBox.mode = DLG_MODE_NLPAUSE;
				nlPause = 0;

				DialogueBox.lineShift = {0,0};

				DBRemoveTopLine();
			}
		} break;

		case DLG_MODE_NLPAUSE:
		{
			if (nlPause < 6)
				nlPause++;
			else
			{
				DialogueBox.mode = DLG_MODE_INPLACE;
			}
		} break;
	}

	DrawDialogBox(GetCounter(counters->counters, DLG_ALPHA), tx->pos);
}

void CreateDialogueBox()
{
	//if there's already a dialog box present, destroy it
	int othId = GetFirstOfType(OBJ_DIALOGUE_BOX);
	if (othId >= 0)
		RemoveObject(othId);

	int goId = AddObject(OBJ_DIALOGUE_BOX);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto counters = GO(counters);
	auto phy = GO(physics);
	meta->cmpInUse = TRANSFORM | PHYSICS | DRAW_GAME_GUI | CUSTOM_FLAGS | COUNTERS;
	GO(draw_game_gui)->draw = DialogueUpdateAndDrawGameGui;

	tx->pos.x = App.gameWindowSize.x << 9;
	tx->pos.y = -100 << 9;
	phy->vel.y = 0x1000;
	SetCounter(&counters->counters, DLG_ALPHA, 0);
	DialogueBox.mode = DLG_MODE_ENTER;

	SetFont("PixelOperator", 15);
	DialogueBox.f = Rend.currentFont;
	DialogueBox.fs = Rend.currentFontSize;

	DialogueBox.s = (game_sprite *)GetFromHash(&Sprites, "DialogueBox");
	poi *p = GetPOI(DialogueBox.s, "noImageUL");
	DialogueBox.startPos = {(((tx->pos.x >> 9) + 2*p->location.x) << 9), ((5 + 2*p->location.y) << 9)};
	DialogueBox.numCharacters = 0;
	DialogueBox.drawTo = 0;
	DialogueBox.displayWidth = ((224 << 1) << 9);
	DialogueBox.lineSize = {0, (-(10 << 1) << 9)};
	DialogueBox.numLines = 3;
}

void RemoveDialogueBox()
{
	int othId = GetFirstOfType(OBJ_DIALOGUE_BOX);
	if (othId >= 0)
		RemoveObject(othId);
}




struct text_dialog
{
	ivec2 size;
	dialogue_character text[256];
	int numCharacters;
	font *f;
	font_size *fs;
};

UPDATE_FUNCTION(DialogDrawGameGui)
{
	auto tx = GO(transform);
	auto dlg = GO(dialog);

	text_dialog *d = (text_dialog *)dlg->data;

	//DrawSprite(DialogueBox.s, WITH_A(COL_WHITE, alpha), {(float)(pos.x >> 9), (float)(pos.y >> 9)}, 0, {2,2});

	//vec2 bl = CoordConversion(vec2(0, 8), &GameGuiVP, &GameWindowProj);
	//vec2 ur = CoordConversion(vec2(2000, 8 + (40 << 1)), &GameGuiVP, &GameWindowProj);
	//EnableScissor(bl.x, ur.x, bl.y, ur.y);
	SetShader("basic_tex_color");

	Rend.currentFont = d->f;
	Rend.currentFontSize = d->fs;
	for (int atlas_index = 0; atlas_index < d->fs->current_atlas + 1; atlas_index++)
	{
		bool needToShift = false;
		texture *t = d->fs->tex[atlas_index];
		Rend.recentlySetTexture = t;
		render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
		cmd->type = RC_SET_TEXTURE;
		cmd->tex = t;
		cmd->tex_slot = 0;
		for (int i = 0; i < d->numCharacters; i++)
		{
			dialogue_character *c = &d->text[i];
			font_character *fc = (font_character *)GetFromHash(&Rend.currentFontSize->chars, c->codepoint);

			if (fc->atlas_index == atlas_index)
			{
				render_command *cmd = &Rend.renderCommands[Rend.currentCommand++];
				cmd->type = RC_ADD_VERTICES;
				cmd->vertex_data_size = 6 * Rend.recentlySetShader->vertex_stride;
				cmd->num_vertices = 6;

				float px_l = (c->pos.x) + (fc->x0 << 8);
				float px_r = (c->pos.x) + (fc->x1 << 8);
				float py_t = (c->pos.y) - (fc->y0 << 8);
				float py_b = (c->pos.y) - (fc->y1 << 8);
				v2 screenTL = ToScreen(px_l, py_t);
				v2 screenBR = ToScreen(px_r, py_b);
				float u0 = fc->u0, u1 = fc->u1, v0 = fc->v0, v1 = fc->v1;
				uint32_t color = Rend.currentFontColor;

				PushVertex(&screenTL.x, &screenTL.y, &color, &u0, &v0);
				PushVertex(&screenBR.x, &screenTL.y, &color, &u1, &v0);
				PushVertex(&screenTL.x, &screenBR.y, &color, &u0, &v1);
				PushVertex(&screenTL.x, &screenBR.y, &color, &u0, &v1);
				PushVertex(&screenBR.x, &screenTL.y, &color, &u1, &v0);
				PushVertex(&screenBR.x, &screenBR.y, &color, &u1, &v1);
			}
		}
	}
	//DisableScissor();

}


int CreateDialog(ivec2 size, ivec2 pos, char *line)
{	
	int goId = AddObject(OBJ_DIALOGUE_BOX);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto counters = GO(counters);
	auto dlg = GO(dialog);
	meta->cmpInUse = TRANSFORM | SPECIAL_DRAW | DIALOG;
	auto draw = GO(special_draw);
	draw->draw = DialogDrawGameGui;
	draw->depth = 100;
	tx->pos = pos;

	dlg->data = malloc(sizeof(text_dialog));
	text_dialog *d = (text_dialog *)dlg->data;
	d->size = size;
	d->numCharacters = 0;

	ivec2 lineSize = {0, (-(10 << 1) << 9)};
	DialogueBox.numLines = 3;
	ivec2 cursorPos = tx->pos;
	ivec2 startPos = tx->pos;
	int curLineNumber = 0;

	SetFont("PixelOperator", 15);
	font *f = Rend.currentFont;
	font_size *fs = Rend.currentFontSize;
	d->f = f;
	d->fs = fs;

	void *str_iter = line;
	int chars_to_draw = utf8len(line);
	for (int i = 0; i < chars_to_draw; i++)
	{
		int c;
		str_iter = utf8codepoint(str_iter, &c);
		font_character *fc = (font_character *)GetFromHash(&fs->chars, c);
		if (fc == NULL)
			fc = AddCharacterToFontSize(fs, c, false);

		//get next word length
		{
			int cPrev = c;
			void *wordLengthIter = str_iter;
			int length = 0;
			while (cPrev && cPrev != ' ')
			{
				int cNext;
				wordLengthIter = utf8codepoint(wordLengthIter, &cNext);
				length += (int)(GetAdvanceForCharacter(f, fs, fc, c, cNext) * 4.0) << 6;
				cPrev = cNext;
			}
			if (cursorPos.x - startPos.x + length > d->size.x)
			{
				//move to next line
				cursorPos = {startPos.x + lineSize.x, cursorPos.y + lineSize.y};
				curLineNumber++;
			}
		}

		dialogue_character *character = &d->text[d->numCharacters++];
		character->codepoint = c;
		character->color = COL_WHITE;
		character->displayTime = 2;
		character->pos = cursorPos;
		character->lineNumber = curLineNumber;

		int cNext;
		utf8codepoint(str_iter, &cNext);
		cursorPos.x += (int)(GetAdvanceForCharacter(f, fs, fc, c, cNext) * 4.0) << 6;
	}

	return goId;
}





#define MB_FRAMES_ALIVE 50

global_variable magic_bullet_trail MagicBulletTrails[150];
global_variable int NumMagicBulletTrails;

UPDATE_FUNCTION(MagicBulletTrailDrawer)
{
	SetShader("basic_tex_color");
	for (int i = 0; i < NumMagicBulletTrails; i++)
	{
		magic_bullet_trail *trail = &MagicBulletTrails[i];
		float scale = (trail->timeLeft / 0.2f);
		DrawSprite("MagicBullet", trail->color, ToScreen(trail->pos.x, trail->pos.y), 0, {scale * 0x200,scale * 0x200});
		trail->timeLeft -= 0.016f;
		if (trail->timeLeft <= 0)
		{
			for (int j = i; j < NumMagicBulletTrails - 1; j++)
			{
				MagicBulletTrails[j] = MagicBulletTrails[j+1];
			}
			NumMagicBulletTrails--;
		}
	}
}

UPDATE_FUNCTION(MagicBulletUpdate)
{
	auto flag = GO(custom_flags); //just use to store color for the trails
	auto counters = GO(counters);
	auto phy = GO(physics);
	auto tx = GO(transform);

	if (tx->scale.x < 1)
	{
		tx->scale = 1.2f * tx->scale;
		if (tx->scale.x > 1)
		{
			tx->scale = {1,1};
		}
	}

	uint8_t rot = GetCounter(counters->counters, MB_ROTATION);
	SetCounter(&counters->counters, MB_ROTATION, rot + 2);
	float rotF = (float)rot * DEGREES_TO_RADIANS;
	float t = rotF * 2 * PI;

	uint8_t sinAmp = GetCounter(counters->counters, MB_SIN_AMP);
	IncrementCounter(&counters->counters, MB_SIN_AMP);
	IncrementCounter(&counters->counters, MB_SIN_AMP);

	phy->vel.y = (int) ((0x100 + sinAmp) * sinf(t));

	IncrementCounter(&counters->counters, MB_TIME_ALIVE);
	uint8_t timeAlive = GetCounter(counters->counters, MB_TIME_ALIVE);
	if (timeAlive > MB_FRAMES_ALIVE)
		RemoveObject(goId);


	DecrementCounter(&counters->counters, MB_NEXT_SMOKE);
	if (GetCounter(counters->counters, MB_NEXT_SMOKE) == 0)
	{
		SetCounter(&counters->counters, MB_NEXT_SMOKE, (rand()%2) + 1);
		if (NumMagicBulletTrails < ArrayCount(MagicBulletTrails))
		{
			magic_bullet_trail *trail = &MagicBulletTrails[NumMagicBulletTrails++];
			trail->pos = tx->pos;
			trail->pos.y += (rand() % (4 << 9)) - (2 << 9);
			trail->color = flag->bits;
			trail->timeLeft = 0.2 * (timeAlive > 15 ? 1 : (timeAlive / 15.0f));
		}
	}

	uint32_t newY;
	if (CollidingWithTileAtPoint(tx->pos.x, tx->pos.y, &newY))
	{
		RemoveObject(goId);
	}

}

void CreateMagicBullet(ivec2 pos, ivec2 vel)
{
	auto goId = AddObject(OBJ_MAGIC_BULLET);
	auto flag = GO(custom_flags);
	auto spr = GO(sprite);
	auto anim = GO(anim);
	auto tx = GO(transform);
	auto update = GO(update);
	auto meta = GO(metadata);
	auto phy = GO(physics);
	auto counters = GO(counters);
	meta->cmpInUse = UPDATE | COUNTERS | TRANSFORM | PHYSICS | SPRITE | CUSTOM_FLAGS;
	meta->flags = GAME_OBJECT;
	spr->name = "MagicBullet";
	spr->depth = 12;

	tx->scale = {0.3f,0.3f};
	tx->pos = pos;
	update->update = MagicBulletUpdate;
	phy->vel = vel;

	SetCounter(&counters->counters, MB_ROTATION, rand() % 255);
	SetCounter(&counters->counters, MB_NEXT_SMOKE, (rand() % 3) + 5);

	flag->bits = MAKE_COLOR((rand() % 50) + 50, (rand() % 100) + 50, (rand() % 130) + 0, 255);
	int whichColor = rand() % 7;
	switch (whichColor)
	{
		case 0: { flag->bits = WITH_R(flag->bits, 0xff); flag->bits = WITH_G(flag->bits, 0xA5); } break; //orange
		case 1:
		case 2:
		case 3: { flag->bits = WITH_R(flag->bits, 0xff); } break; //red
		case 4: { flag->bits = WITH_R(flag->bits, 0xff); flag->bits = WITH_B(flag->bits, 0xff); } break; //purple?
		case 5: { flag->bits = WITH_G(flag->bits, 0xff); } break; //green
		case 6: { flag->bits = WITH_B(flag->bits, 0xff); } break; //blue
	}

	spr->color = COL_WHITE;
}

struct basic_bullet
{
	ivec2 pos;
	ivec2 vel;
	int timeLeft;
};

global_variable basic_bullet BasicBullets[100];
global_variable int NumBasicBullets;

UPDATE_FUNCTION(BasicBulletUpdateAndDraw)
{
	for (int i = 0; i < NumBasicBullets; i++)
	{
		basic_bullet *bullet = &BasicBullets[i];
		bullet->timeLeft--;

		int steps = Abs(bullet->vel.x) >> 10;
		int distance = bullet->vel.x > 0 ? 0x400 : -0x400;

		uint32_t start = bullet->pos.x;
		bool collision = false;
		for (int j = 0; j < steps; j++)
		{
			if (CollidingWithTileAtPoint(start, bullet->pos.y, NULL) || CollidingWithTileAtPoint(start, bullet->pos.y - (2 << 9), NULL))
			{
				collision = true;
				break;
			}
			start += distance;
		}
		if (collision || bullet->timeLeft <= 0)
		{
			BasicBullets[i] = BasicBullets[--NumBasicBullets];
			i--;
		}
		else
		{
			bullet->pos += bullet->vel;
			DrawSprite("BasicBullet", COL_WHITE, ToScreen(bullet->pos.x, bullet->pos.y), 0, {(float)Sign(bullet->vel.x) * 0x200, 0x200});
		}
	}
}

void CreateBasicBullet(ivec2 pos, ivec2 vel)
{
	if (NumBasicBullets < ArrayCount(BasicBullets))
	{
		basic_bullet *bullet = &BasicBullets[NumBasicBullets++];
		bullet->pos = pos;
		bullet->vel = vel;
		bullet->timeLeft = 40;
	}
}

enum point_type
{
	POINT_TOP,
	POINT_BOTTOM,
	POINT_LEFT,
	POINT_RIGHT,
};

struct test_point
{
	bool hitting;
	ivec2 p;
	point_type type;
	int32_t limit;
	vec2 normal;
	int platformID;
};

void PlatformTest(int goId, test_point *points, int numPoints)
{
	uint32_t minX = INT_MAX;
	uint32_t minY = INT_MAX;
	uint32_t maxX = INT_MIN;
	uint32_t maxY = INT_MIN;

	for (int i = 0; i < numPoints; i++)
	{
		if (points[i].p.x < minX)
			minX = points[i].p.x;
		if (points[i].p.y < minY)
			minY = points[i].p.y;
		if (points[i].p.x > maxX)
			maxX = points[i].p.x;
		if (points[i].p.y > maxY)
			maxY = points[i].p.y;
        
        points[i].hitting = false;

		switch (points[i].type)
		{
			case POINT_TOP:		{ points[i].limit = INT_MAX; } break;
			case POINT_BOTTOM:	{ points[i].limit = INT_MIN; } break;
			case POINT_LEFT:	{ points[i].limit = INT_MIN; } break;
			case POINT_RIGHT:	{ points[i].limit = INT_MAX; } break;
		}
	}

	for (int j = 0; j < NumGameObjects; j++)
	{
		if (GameComponents.idIndex[j] == goId)
			continue;

		auto meta = &GameComponents.metadata[j];
		if ((meta->cmpInUse & MOVING_PLATFORM) && (meta->cmpInUse & TRANSFORM))
		{
			auto platform = &GameComponents.moving_platform[j];
			auto platformTx = &GameComponents.transform[j];
			uint32_t platXL = platformTx->pos.x + platform->bl.x;
			uint32_t platXR = platformTx->pos.x + platform->ur.x;
			uint32_t platYB = platformTx->pos.y + platform->bl.y;
			uint32_t platYT = platformTx->pos.y + platform->ur.y;

			//check AABB first to see if we should even bother checking points
			if (minX > platXR || minY > platYT || maxX < platXL || maxY < platYB)
				continue;

			ivec2 platBL = {platXL, platYB};
			ivec2 platUR = {platXR, platYT};
			for (int i = 0; i < numPoints; i++)
			{
				if (PointInAABB(points[i].p, platBL, platUR))
				{
					test_point *point = &points[i];
					points[i].hitting = true;
					switch (points[i].type)
					{
						case POINT_TOP: 
						{ 
							if (points[i].limit > (platformTx->pos.y + platform->bl.y))
							{
								points[i].limit = platformTx->pos.y + platform->bl.y;
								points[i].normal = {0,-1};
								points[i].platformID = GameComponents.idIndex[j];
							}
						} break;
						case POINT_BOTTOM: 
						{ 
							if (points[i].limit < (platformTx->pos.y + platform->ur.y))
							{
								points[i].limit = (platformTx->pos.y + platform->ur.y); 
								points[i].normal = {0,1};
								points[i].platformID = GameComponents.idIndex[j];
							}
						} break;
						case POINT_RIGHT: 
						{
							if (points[i].limit > platformTx->pos.x + platform->bl.x)
							{
								points[i].limit = platformTx->pos.x + platform->bl.x;
								points[i].normal = {-1,0};
								points[i].platformID = GameComponents.idIndex[j];
							}
						} break;
						case POINT_LEFT: 
						{
							if (points[i].limit < platformTx->pos.x + platform->ur.x)
							{
								points[i].limit = platformTx->pos.x + platform->ur.x;
								points[i].normal = {1,0};
								points[i].platformID = GameComponents.idIndex[j];
							}
						} break;
					}
				}
			}
		}
	}
}

//must run platform test first
void TileTest(test_point *points, int numPoints)
{
	for (int i = 0; i < numPoints; i++)
	{
		test_point *p = &points[i];
		int32_t tileX = (p->p.x >> 12) << 12;
		int32_t tileY = (p->p.y >> 12) << 12;

		uint8_t t = TileAtPoint({tileX, tileY});
		switch (p->type)
		{
			case POINT_BOTTOM:
			{
				if (t != EMPTY)
				{
					int limit = GetGroundYForTileAndPos(tileX, tileY, t, p->p.x);
					if (limit > p->limit && limit > p->p.y)
					{
						p->hitting = true;
						p->limit = limit;
						switch (t)
						{
							case SQUARE: { p->normal = {0,1}; } break;
							case SMALL_SLOPE_LEFT1:
							case SMALL_SLOPE_LEFT2: { p->normal = normalize(vec2(1,2)); } break;
							case SMALL_SLOPE_RIGHT1:
							case SMALL_SLOPE_RIGHT2:  { p->normal = normalize(vec2(-1,2)); } break;
							case LARGE_SLOPE_RIGHT: { p->normal = normalize(vec2(-1,1)); } break;
							case LARGE_SLOPE_LEFT: { p->normal = normalize(vec2(1,1)); } break;
						}
					}
				}
			} break;
			case POINT_LEFT:
			{
				if (t == SQUARE)
				{
					int limit = tileX + (1 << 12);
					if (limit > p->limit)
					{
						p->hitting = true;
						p->limit = limit;
					}
				}
			} break;
			case POINT_RIGHT:
			{
				if (t == SQUARE)
				{
					int limit = tileX;
					if (limit < p->limit)
					{
						p->hitting = true;
						p->limit = limit;
					}
				}
			} break;
			case POINT_TOP:
			{
				if (t != EMPTY)
				{
					int limit = tileY;
					if (limit < p->limit)
					{
						p->hitting = true;
						p->limit = limit;
					}
				}
			} break;
		}
	}
}

/*
enum upgrade_type 
{
	INCREASE_GENERATOR_POWER,
	INCREASE_GENERATOR_EFFICIENCY,

	UPGRADE_COUNT,
};

struct upgrade_level
{
	char *description;
	char *sprite;
	int value;
	int rawMaterial;
	int energy;
};

struct game_upgrade
{
	bool available;
	upgrade_type type;
	upgrade_level levels[10];
	int numLevels;
	int currentLevel;
};

struct game_info
{
	int generatorEfficiency = 3;
	int generatorPower = 1;
	int storedEnergy = 0;
	int rawMaterial = 2000;

	game_upgrade upgrades[UPGRADE_COUNT] = {
		{
			true,
			INCREASE_GENERATOR_POWER,
			{
				{
					"Creates low quality crystals",
					"GeneratorLightUpgrade",
					1
				},
				{
					"Creates higher value crystals",
					"GeneratorLightUpgrade",
					2,
					100,
					5,
				},
				{
					"Creates even higher value crystals",
					"GeneratorLightUpgrade",
					3,
					100,
					5,
				},
				{
					"Creates very high value crystals",
					"GeneratorLightUpgrade",
					4,
					100,
					5,
				},
				{
					"Creates extremely high value crystals",
					"GeneratorLightUpgrade",
					5,
					100,
					5,
				},
				{
					"Creates the highest quality crystals",
					"GeneratorLightUpgrade",
					6,
					100,
					5,
				},
			},
			6,
			0,
		},
		{
			true,
			INCREASE_GENERATOR_EFFICIENCY,
			{
				{
					"Plastic actuator",
					"ActuatorUpgrade",
					2,
					100,
					5,
				},
				{
					"Aluminum actuator",
					"ActuatorAluminumUpgrade",
					5,
					100,
					5,
				},
				{
					"Graphite actuator",
					"ActuatorGraphiteUpgrade",
					7,
					100,
					5,
				},
				{
					"Gold-plated Carbide actuator",
					"ActuatorGoldUpgrade",
					9,
					100,
					5,
				},
			},
			4,
			0,
		},
	};
};

global_variable game_info game;

UPDATE_FUNCTION(CrystalAcceptorUpdate)
{
	auto collider = GO(collider);
	auto counters = GO(counters);

	for (int i = 0; i < ArrayCount(collider->collisions); i++)
	{
		int othID = collider->collisions[i];
		if (othID != -1)
		{
			uint8_t slots = GetCounter(counters->counters, 0);
			uint8_t filled = GetCounter(counters->counters, 1);
			object_type type = GameComponents.type[GameObjectIDs[othID].index];
			if (type == OBJ_CRYSTAL && filled < slots)
			{
				auto othcounters = OTH(othID, counters);
				uint8_t crystalVal = GetCounter(othcounters->counters, 0);
				RemoveObject(othID);
				if (filled + crystalVal > slots)
					crystalVal = slots - filled;
				IncrementCounterByAmountNoLoop(&counters->counters, 1, crystalVal);
				game.storedEnergy += crystalVal;
			}
		}
		else
			break;
	}
}

UPDATE_FUNCTION(CrystalAcceptorDraw)
{
	auto counters = GO(counters);
	auto tx = GO(transform);
	SetShader("basic_tex_color");
	vec2 screenPos = ToScreen(tx->pos.x, tx->pos.y);

	game_sprite *s = (game_sprite *)GetFromHash(&Sprites, "CrystalAcceptor");
	DrawSprite(s, COL_WHITE, screenPos, 0, {0x200, 0x200});

	poi *p = GetPOI(s, "text");
	ivec2 attachPoint = p->location;

	vec2 textPos = screenPos + vec2(p->location.x << 9, p->location.y << 9);
	SetFont("PixelOperator", 15);
	SetFontColor(MAKE_COLOR(255,200,200,255));
	char message[32];
	uint8_t slots = GetCounter(counters->counters, 0);
	uint8_t filled = GetCounter(counters->counters, 1);
	sprintf(message, "%d/%d", filled, slots);
	_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
}


UPDATE_FUNCTION(CrystalUpdate)
{
	auto tx = GO(transform);
	auto phy = GO(physics);
	test_point TestPoints[4] = {};
	TestPoints[0].p = {tx->pos.x, tx->pos.y + (10 << 9)};
	TestPoints[0].type = POINT_TOP;

	PlatformTest(TestPoints, 1);
	TileTest(TestPoints, 1);

	if (TestPoints[0].hitting)
	{
		tx->pos.y = TestPoints[0].limit - (11 << 9);
		if (phy->vel.y > 0)
			phy->vel.y *= -0.8f;
	}

	TestPoints[0].p = {tx->pos.x, tx->pos.y - 0x5};
	TestPoints[0].type = POINT_BOTTOM;
	TestPoints[1].p = {tx->pos.x, tx->pos.y + 0x200};
	TestPoints[1].type = POINT_BOTTOM;
	PlatformTest(TestPoints, 2);
	TileTest(TestPoints, 2);

	int horizontalDamping = (TestPoints[0].hitting) ? 6 : 2;
	if (TestPoints[0].hitting && !TestPoints[1].hitting)
	{
		tx->pos.y = TestPoints[0].limit + 0x5;
		float norm = Inner(TestPoints[0].normal, vec2(phy->vel.x, phy->vel.y));
		ivec2 normComponent = norm * TestPoints[0].normal;
		ivec2 tangComponent = phy->vel - normComponent;
		if (LengthSq(vec2(tangComponent)) < 0x60*0x60)
			tangComponent = {0,0};
		normComponent = normComponent * -8 / 10;
		phy->vel = tangComponent + normComponent;
	}
	else if (TestPoints[1].hitting)
	{
		tx->pos.y = TestPoints[1].limit;
		float norm = Inner(TestPoints[1].normal, vec2(phy->vel.x, phy->vel.y));
		ivec2 normComponent = norm * TestPoints[1].normal;
		ivec2 tangComponent = phy->vel - normComponent;
		if (LengthSq(vec2(tangComponent)) < 0x60*0x60)
			tangComponent = {0,0};
		normComponent = normComponent * -8 / 10;
		phy->vel = tangComponent + normComponent;

	}

	TestPoints[0].p = {tx->pos.x - (4 << 9), tx->pos.y + (4 << 9)};
	TestPoints[0].type = POINT_LEFT;
	TestPoints[1].p = {tx->pos.x - (4 << 9), tx->pos.y + (10 << 9)};
	TestPoints[1].type = POINT_LEFT;
	TestPoints[2].p = {tx->pos.x + (4 << 9), tx->pos.y + (4 << 9)};
	TestPoints[2].type = POINT_RIGHT;
	TestPoints[3].p = {tx->pos.x + (4 << 9), tx->pos.y + (10 << 9)};
	TestPoints[3].type = POINT_RIGHT;
	PlatformTest(TestPoints, 4);
	TileTest(TestPoints, 4);

	int limitLeft = Max(TestPoints[0].limit, TestPoints[1].limit);
	if (TestPoints[0].hitting || TestPoints[1].hitting)
	{
		tx->pos.x = limitLeft + (5 << 9);
		if (phy->vel.x < 0)
			phy->vel.x *= -0.8f;
	}

	int limitRight = Min(TestPoints[2].limit, TestPoints[3].limit);
	if (TestPoints[2].hitting || TestPoints[3].hitting)
	{
		tx->pos.x = limitRight - (5 << 9);
		if (phy->vel.x > 0)
			phy->vel.x *= -0.8f;
	}

	int verticalDamping = 2;
	if (phy->vel.y >= verticalDamping)
		phy->vel.y -= verticalDamping;
	else if (phy->vel.y <= verticalDamping)
		phy->vel.y += verticalDamping;
	else
		phy->vel.y = 0;


	if (phy->vel.x > horizontalDamping)
		phy->vel.x -= horizontalDamping;
	else if (phy->vel.x < -horizontalDamping)
		phy->vel.x += horizontalDamping;
	else
		phy->vel.x = 0;

	phy->vel.x = ClampToRange(-0x700,phy->vel.x,0x700);
	phy->vel.y = ClampToRange(-0x700,phy->vel.y,0x700);
}

void CreatePowerCrystal(ivec2 pos, uint8_t energy, ivec2 vel)
{
	int goId = AddObject(OBJ_CRYSTAL);
	auto meta = GO(metadata);
	meta->cmpInUse = SPRITE | UPDATE | TRANSFORM | PHYSICS | RIDES_PLATFORMS | COUNTERS | COLLIDER;
	meta->flags = GAME_OBJECT;
	InitObject(goId);
	auto spr = GO(sprite);
	spr->name = "Crystal";
	spr->depth = 10;
	auto update = GO(update);
	update->update = CrystalUpdate;
	auto tx = GO(transform);
	tx->pos = pos;
	auto phy = GO(physics);
	phy->vel = vel;
	phy->accel = {0,-30};
	auto counters = GO(counters);
	SetCounter(&counters->counters, 0, energy);
	auto collider = GO(collider);
	collider->bl = {-2048, 0};
	collider->ur = {2048,4096};
}

global_variable int Gen_LightObjects[6];
global_variable int Gen_NumLightObjects;

global_variable int Gen_CrystalDisplay;
global_variable int Gen_EjectingCrystal;
global_variable int Gen_EmptyCapsule;
global_variable int Gen_FilledCapsule;

global_variable int Gen_FilledPercentage;
global_variable bool Gen_LastWasLeft;

global_variable ivec2 Gen_StartFill;
global_variable ivec2 Gen_EndFill;
global_variable ivec2 Gen_ActuatorPos;
global_variable ivec2 Gen_ActuatorCurrentPos;
global_variable ivec2 Gen_ActuatorLeft;
global_variable ivec2 Gen_ActuatorRight;

UPDATE_FUNCTION(GeneratorUpdateAndDraw)
{
	auto counters = GO(counters);
	bool filled = false;
	if (Gen_FilledPercentage == -1)
	{
		if (KeyPresses[KEY_Left])
		{
			Gen_LastWasLeft = true;
			Gen_FilledPercentage = 0;
			SetCounter(&counters->counters, 0, 0); //time since last added energy
			Gen_ActuatorPos = Gen_ActuatorLeft;
		}
		else if (KeyPresses[KEY_Right])
		{
			Gen_LastWasLeft = false;
			Gen_FilledPercentage = 0;
			SetCounter(&counters->counters, 0, 0); //time since last added energy
			Gen_ActuatorPos = Gen_ActuatorRight;
		}
	}
	else
	{
		if (Gen_FilledPercentage < 100 && ((Gen_LastWasLeft && KeyPresses[KEY_Right]) || (!Gen_LastWasLeft && KeyPresses[KEY_Left])))
		{
			Gen_FilledPercentage += game.generatorEfficiency;
			game.rawMaterial -= 1;
			if (Gen_FilledPercentage >= 100)
			{
				filled = true;
				Gen_FilledPercentage = 100;
			}
			Gen_LastWasLeft = !Gen_LastWasLeft;
			Gen_ActuatorPos = Gen_LastWasLeft ? Gen_ActuatorLeft : Gen_ActuatorRight;
			SetCounter(&counters->counters, 0, 0); //time since last added energy
		}
	}

	IncrementCounterNoLoop(&counters->counters, 0);
	uint8_t timeSinceLastAddedEnergy = GetCounter(counters->counters, 0);

	auto crystalAnim = OTH(Gen_CrystalDisplay, anim);
	crystalAnim->playing = timeSinceLastAddedEnergy < 30;
	if (!crystalAnim->playing)
	{
		crystalAnim->currentFrame = 0;
		crystalAnim->frameTime = 0;
		crystalAnim->playing = true;
		AnimationUpdate(Gen_CrystalDisplay);
		crystalAnim->playing = false;
	}
	auto tx = GO(transform);

	if (filled)
	{
		//Eject crystal
		game_sprite *s = (game_sprite *)GetFromHash(&Sprites, "GeneratorPanel");

		{
			int goId = AddObject(OBJ_NONE);
			auto meta = GO(metadata);
			meta->cmpInUse = ANIM | SPRITE | TRANSFORM;
			InitObject(goId);
			auto spr = GO(sprite);
			spr->depth = 99;
			auto anim = GO(anim);
			anim->name = "ejecting_crystal";
			AnimationUpdate(goId);
			anim->loop = false;
			auto crystalTx = GO(transform);

			poi *p = GetPOI(s, "crystal");
			ivec2 attachPoint = p->location;
			crystalTx->pos = {tx->pos.x + (attachPoint.x << 9), tx->pos.y + (attachPoint.y << 9)};

			Gen_EjectingCrystal = goId;
		}
	}
	else if (Gen_FilledPercentage == 100)
	{
		auto anim = OTH(Gen_EjectingCrystal, anim);
		if (anim->playing == false) //ejecting crystal animation is done
		{
			int genId = GetFirstOfType(OBJ_CRYSTAL_GENERATOR);
			auto genTx = OTH(genId, transform);
			CreatePowerCrystal(genTx->pos, Gen_NumLightObjects, {(rand() % 300) - 150, (rand() % 80) + 40});

			action *a = AddStartingAction(RETURN_CHARACTER_CONTROL);
			AddSubsequentAction(&a, UNFREEZE_CAMERA);
			RemoveObject(goId);
			RemoveObject(Gen_FilledCapsule);
			RemoveObject(Gen_EmptyCapsule);
			RemoveObject(Gen_CrystalDisplay);
			RemoveObject(Gen_EjectingCrystal);
			for (int i = 0; i < Gen_NumLightObjects; i++)
			{
				RemoveObject(Gen_LightObjects[i]);
			}
		}
	}

	auto filltx = OTH(Gen_FilledCapsule, transform);
	ivec2 fillTo = ((Gen_EndFill - Gen_StartFill) * Gen_FilledPercentage / 100) + Gen_StartFill;
	int moveX = (fillTo.x - filltx->pos.x) >> 2;
	int moveY = (fillTo.y - filltx->pos.y) >> 2;
	filltx->pos.x += moveX;
	filltx->pos.y += moveY;

	moveX = (Gen_ActuatorPos.x - Gen_ActuatorCurrentPos.x) >> 1;
	moveY = (Gen_ActuatorPos.y - Gen_ActuatorCurrentPos.y) >> 1;
	Gen_ActuatorCurrentPos.x += moveX;
	Gen_ActuatorCurrentPos.y += moveY;

	DrawSprite("GeneratorPanel", COL_WHITE, ToScreen(tx->pos.x, tx->pos.y), 0, {0x200, 0x200});
	DrawSprite("Actuator", COL_WHITE, ToScreen(Gen_ActuatorCurrentPos), 0, {0x200, 0x200});
}

void CreateGeneratorDisplay()
{
	Gen_FilledPercentage = -1;

	int goId = AddObject(OBJ_GENERATOR_DISPLAY);
	auto meta = GO(metadata);
	meta->cmpInUse = SPECIAL_DRAW | TRANSFORM;
	InitObject(goId);
	meta->flags = GAME_OBJECT;
	auto draw = GO(special_draw);
	draw->draw = GeneratorUpdateAndDraw;
	draw->depth = 100;
	auto tx = GO(transform);
	tx->pos = Camera.pos;

	Gen_NumLightObjects = game.generatorPower;

	game_sprite *s = (game_sprite *)GetFromHash(&Sprites, "GeneratorPanel");

	for (int i = 0; i < Gen_NumLightObjects; i++)
	{
		int goId = AddObject(OBJ_NONE);
		auto meta = GO(metadata);
		meta->cmpInUse = ANIM | SPRITE | TRANSFORM;
		InitObject(goId);
		auto spr = GO(sprite);
		spr->depth = 101;
		auto anim = GO(anim);
		anim->name = "generator_light";
		AnimationUpdate(goId);
		auto lightTx = GO(transform);

		char lightName[10];
		sprintf(lightName, "light%d", i);
		poi *p = GetPOI(s, lightName);
		ivec2 attachPoint = p->location;
		lightTx->pos = {tx->pos.x + (attachPoint.x << 9), tx->pos.y + (attachPoint.y << 9)};

		Gen_LightObjects[i] = goId;
	}


	//create the crystal display
	{
		int goId = AddObject(OBJ_NONE);
		auto meta = GO(metadata);
		meta->cmpInUse = ANIM | SPRITE | TRANSFORM;
		InitObject(goId);
		auto spr = GO(sprite);
		spr->depth = 98;
		auto anim = GO(anim);
		anim->name = "lightning_crystal";
		AnimationUpdate(goId);
		anim->playing = false;
		auto crystalTx = GO(transform);

		poi *p = GetPOI(s, "crystal");
		ivec2 attachPoint = p->location;
		crystalTx->pos = {tx->pos.x + (attachPoint.x << 9), tx->pos.y + (attachPoint.y << 9)};

		Gen_CrystalDisplay = goId;
	}


	//create the capsule display
	{
		int goId = AddObject(OBJ_NONE);
		auto meta = GO(metadata);
		meta->cmpInUse = SPRITE | TRANSFORM;
		InitObject(goId);
		auto spr = GO(sprite);
		spr->depth = 98;
		spr->name = "EmptyGeneratorCapsule";
		auto capsuleTx = GO(transform);

		poi *p = GetPOI(s, "capsule");
		ivec2 attachPoint = p->location;
		capsuleTx->pos = {tx->pos.x + (attachPoint.x << 9), tx->pos.y + (attachPoint.y << 9)};

		Gen_EmptyCapsule = goId;
	}


	//create the capsule display
	{
		int goId = AddObject(OBJ_NONE);
		auto meta = GO(metadata);
		meta->cmpInUse = SPRITE | TRANSFORM;
		InitObject(goId);
		auto spr = GO(sprite);
		spr->depth = 99;
		spr->name = "FilledGeneratorCapsule";
		auto capsuleTx = GO(transform);

		poi *p = GetPOI(s, "capsule");
		ivec2 attachPoint = p->location;
		capsuleTx->pos = {tx->pos.x + (attachPoint.x << 9), tx->pos.y + (attachPoint.y << 9)};
		Gen_StartFill = capsuleTx->pos;

		poi *p2 = GetPOI(s, "filled");
		ivec2 filledPoint = p2->location;
		Gen_EndFill = {tx->pos.x + (filledPoint.x << 9), tx->pos.y + (filledPoint.y << 9)};

		Gen_FilledCapsule = goId;
	}

	poi *p = GetPOI(s, "actuatormid");
	ivec2 actuator = p->location;
	Gen_ActuatorPos = {tx->pos.x + (actuator.x << 9), tx->pos.y + (actuator.y << 9)};
	Gen_ActuatorCurrentPos = Gen_ActuatorPos;

	p = GetPOI(s, "actuatorleft");
	actuator = p->location;
	Gen_ActuatorLeft = {tx->pos.x + (actuator.x << 9), tx->pos.y + (actuator.y << 9)};

	p = GetPOI(s, "actuatorright");
	actuator = p->location;
	Gen_ActuatorRight = {tx->pos.x + (actuator.x << 9), tx->pos.y + (actuator.y << 9)};

	action *a = AddStartingAction(REMOVE_CHARACTER_CONTROL);
	AddSubsequentAction(&a, FREEZE_CAMERA);

}

enum upgrade_panel_mode
{
	UPGRADE_PICKER,
	UPGRADE_SHOW,
	UPGRADE_CONFIRM,
	UPGRADE_CANCEL,
};

global_variable upgrade_panel_mode Up_Mode;

UPDATE_FUNCTION(UpgradePanelUpdateAndDraw)
{
	if (KeyPresses[KEY_Action2])
	{
		if (Up_Mode == UPGRADE_PICKER)
		{
			RemoveObject(goId);
			action *a = AddStartingAction(RETURN_CHARACTER_CONTROL);
			AddSubsequentAction(&a, UNFREEZE_CAMERA);
		}
		else if (Up_Mode == UPGRADE_CONFIRM || Up_Mode == UPGRADE_CANCEL)
			Up_Mode = UPGRADE_SHOW;
		else
			Up_Mode = UPGRADE_PICKER;
	}
	auto tx = GO(transform);
	game_sprite *s;
	local_persistent int selectedUpgrade = 0;

	if (Up_Mode == UPGRADE_PICKER)
	{
		s = (game_sprite *)GetFromHash(&Sprites, "UpgradePanel");
		SetShader("basic_tex_color");
		DrawSprite(s, COL_WHITE, ToScreen(tx->pos.x, tx->pos.y), 0, {0x200, 0x200});
		SetFont("PixelOperator", 15);
		SetFontColor(MAKE_COLOR(222,220,191,255));

		{
			poi *p = GetPOI(s, "title");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			vec2 textPos = ToScreen(tx->pos + loc);
			_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, "CHOOSE AN UPGRADE");
		}

		int upgradeCount = 0;

		SetTextAlignment(ALIGN_CENTER);
		for (int i = 0; i < ArrayCount(game.upgrades); i++)
		{
			game_upgrade *upgrade = &game.upgrades[i];
			if (upgrade->available)
			{
				upgradeCount++;
				{
					char poiname[32];
					sprintf(poiname, "upgrade%d", i);
					poi *p = GetPOI(s, poiname);
					ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
					vec2 pos = ToScreen(tx->pos + loc);
					DrawSprite(upgrade->levels[upgrade->currentLevel].sprite, COL_WHITE, pos, 0, {0x200,0x200});

					if (selectedUpgrade == upgradeCount - 1)
					{
						DrawSprite("UpgradeSelector", COL_WHITE, pos, 0, {0x200, 0x200});
					}
				}

				{
					char poiname[32];
					sprintf(poiname, "upgrade%dtext", i);
					poi *p = GetPOI(s, poiname);
					ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
					vec2 textPos = ToScreen(tx->pos + loc);
					char message[16];
					if (upgrade->currentLevel == upgrade->numLevels - 1)
						sprintf(message, "LV MAX");
					else
						sprintf(message, "LV.%d",upgrade->currentLevel + 1);
					_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
				}
			}
		}

		if (KeyPresses[KEY_Left])
			selectedUpgrade = Max(0, selectedUpgrade-1);
		else if (KeyPresses[KEY_Right])
			selectedUpgrade = Min(upgradeCount-1,selectedUpgrade+1);

		{
			SetTextAlignment(ALIGN_LEFT);
			poi *p = GetPOI(s, "description");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			vec2 textPos = ToScreen(tx->pos + loc);
			game_upgrade *upgrade = &game.upgrades[selectedUpgrade];
			char *message = upgrade->levels[upgrade->currentLevel].description;
			_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
		}

		if (KeyPresses[KEY_Action1])
		{
			game_upgrade *upgrade = &game.upgrades[selectedUpgrade];
			if (upgrade->currentLevel < upgrade->numLevels - 1)
			{
				Up_Mode = UPGRADE_SHOW;
			}
		}

	}
	else
	{
		s = (game_sprite *)GetFromHash(&Sprites, "ConfirmUpgradePanel");
		SetShader("basic_tex_color");
		DrawSprite(s, COL_WHITE, ToScreen(tx->pos.x, tx->pos.y), 0, {0x200, 0x200});
		SetFont("PixelOperator", 15);
		SetFontColor(MAKE_COLOR(222,220,191,255));

		game_upgrade *upgrade = &game.upgrades[selectedUpgrade];
		
		{
			poi *p = GetPOI(s, "upgradefrom");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			vec2 screenPos = ToScreen(tx->pos + loc);
			DrawSprite(upgrade->levels[upgrade->currentLevel].sprite, COL_WHITE, screenPos, 0, {0x200, 0x200});
		}
		
		{
			poi *p = GetPOI(s, "upgradeto");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			vec2 screenPos = ToScreen(tx->pos + loc);
			DrawSprite(upgrade->levels[upgrade->currentLevel + 1].sprite, COL_WHITE, screenPos, 0, {0x200, 0x200});
		}


		SetTextAlignment(ALIGN_CENTER);
		{
			poi *p = GetPOI(s, "materialcost");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			vec2 textPos = ToScreen(tx->pos + loc);
			char message[16];
			sprintf(message, "%d", upgrade->levels[upgrade->currentLevel+1].rawMaterial);
			SetFontColor(COL_BLACK);
			_Rendering::DrawTextConvertedSize(textPos.x + 0x100, textPos.y, 0x100, 0x100, message);
			_Rendering::DrawTextConvertedSize(textPos.x, textPos.y - 0x100, 0x100, 0x100, message);
			SetFontColor(MAKE_COLOR(222,220,191,255));
			_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
		}

		{
			poi *p = GetPOI(s, "energycost");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			vec2 textPos = ToScreen(tx->pos + loc);
			char message[16];
			sprintf(message, "%d", upgrade->levels[upgrade->currentLevel+1].energy);
			SetFontColor(COL_BLACK);
			_Rendering::DrawTextConvertedSize(textPos.x + 0x100, textPos.y, 0x100, 0x100, message);
			_Rendering::DrawTextConvertedSize(textPos.x, textPos.y - 0x100, 0x100, 0x100, message);
			SetFontColor(MAKE_COLOR(222,220,191,255));
			_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
		}

		vec2 confirmPos;
		{
			poi *p = GetPOI(s, "confirm");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			confirmPos = ToScreen(tx->pos + loc);
		}
		vec2 cancelPos;
		{
			poi *p = GetPOI(s, "cancel");
			ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
			cancelPos = ToScreen(tx->pos + loc);
		}

		if (Up_Mode == UPGRADE_CONFIRM)
		{
			DrawSprite("MenuSelector", MAKE_COLOR(106,190,48,255), confirmPos, 0, {0x200, 0x200});
		}
		else if (Up_Mode == UPGRADE_CANCEL)
		{
			DrawSprite("MenuSelector", MAKE_COLOR(217,87,99,255), cancelPos, 0, {0x200, 0x200});
		}


		_Rendering::DrawTextConvertedSize(confirmPos.x, confirmPos.y, 0x100, 0x100, "Confirm");
		_Rendering::DrawTextConvertedSize(cancelPos.x, cancelPos.y, 0x100, 0x100, "Cancel");

		switch (Up_Mode)
		{
			case UPGRADE_SHOW:
			{
				if (KeyPresses[KEY_Action1] || KeyPresses[KEY_Down])
					Up_Mode = UPGRADE_CONFIRM;
			} break;

			case UPGRADE_CONFIRM:
			{
				if (KeyPresses[KEY_Action1])
				{
					Up_Mode = UPGRADE_PICKER;
					switch (upgrade->type)
					{
						case INCREASE_GENERATOR_POWER:
						{
							game.generatorPower = upgrade->levels[++upgrade->currentLevel].value;
						} break;

						case INCREASE_GENERATOR_EFFICIENCY:
						{
							game.generatorEfficiency = upgrade->levels[++upgrade->currentLevel].value;
						} break;

						case UPGRADE_COUNT: {} break;
					}
				}
				else if (KeyPresses[KEY_Down])
					Up_Mode = UPGRADE_CANCEL;
				else if (KeyPresses[KEY_Up])
					Up_Mode = UPGRADE_SHOW;
			} break;

			case UPGRADE_CANCEL:
			{
				if (KeyPresses[KEY_Action1])
					Up_Mode = UPGRADE_PICKER;
				else if (KeyPresses[KEY_Up])
					Up_Mode = UPGRADE_CONFIRM;
			} break;

			default:  {} break;
		}
	}

	SetTextAlignment(ALIGN_CENTER);
	{
		poi *p = GetPOI(s, "rawmaterial");
		ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
		vec2 textPos = ToScreen(tx->pos + loc);
		char message[16];
		sprintf(message, "%d", game.rawMaterial);
		_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
	}

	{
		poi *p = GetPOI(s, "energy");
		ivec2 loc = {((int)p->location.x)<<9, ((int)p->location.y)<<9};
		vec2 textPos = ToScreen(tx->pos + loc);
		char message[16];
		sprintf(message, "%d", game.storedEnergy);
		_Rendering::DrawTextConvertedSize(textPos.x, textPos.y, 0x100, 0x100, message);
	}
}

void CreateUpgradePanel()
{
	Up_Mode = UPGRADE_PICKER;

	int goId = AddObject(OBJ_UPGRADE_PANEL);
	auto meta = GO(metadata);
	meta->cmpInUse = SPECIAL_DRAW | TRANSFORM;
	InitObject(goId);
	meta->flags = GAME_OBJECT;
	auto draw = GO(special_draw);
	draw->draw = UpgradePanelUpdateAndDraw;
	draw->depth = 100;
	auto tx = GO(transform);
	tx->pos = Camera.pos;

	action *a = AddStartingAction(REMOVE_CHARACTER_CONTROL);
	AddSubsequentAction(&a, FREEZE_CAMERA);
}
*/


struct tile_collision_info
{
	int maxFallSpeed = -0x600;
	int headHalfWidth = 3072; // 6 << 9
	int headHeight = 6144; // 12 << 9
	int bodyHalfWidth = 4096; // 8 << 9

	int gravity = -36;
	int maxVelocity = 0x400;

	int horizontalDamping = 20;
	int airborneHorizontalDamping = 6;

	bool setXScale = true;
};

void EnemyTileCollisions(int goId, tile_collision_info *info)
{
	auto phys = GO(physics);
	auto tx = GO(transform);
	auto flag = GO(custom_flags);
	auto rider = GO(rides_platforms);
	
	phys->vel.y = Max(info->maxFallSpeed, phys->vel.y);

	bool wasOnGround = flag->bits & ON_GROUND;
	flag->bits &= ~ON_GROUND;

	test_point TestPoints[11] = {};
	TestPoints[0].p = {tx->pos.x, tx->pos.y - (1 << 9)};
	TestPoints[0].type = POINT_BOTTOM;
	TestPoints[1].p = {tx->pos.x - (3 << 9), tx->pos.y - (1 << 9)};
	TestPoints[1].type = POINT_BOTTOM;
	TestPoints[2].p = {tx->pos.x + (3 << 9), tx->pos.y - (1 << 9)};
	TestPoints[2].type = POINT_BOTTOM;

	PlatformTest(goId, TestPoints + 0, 3);

	uint8_t centerFootT = TileAtPoint(TestPoints[0].p, tx->oldY);
	uint8_t leftFootT = TileAtPoint(TestPoints[1].p, tx->oldY);
	uint8_t rightFootT = TileAtPoint(TestPoints[2].p, tx->oldY);

	tx->oldY = tx->pos.y;

	int32_t tileH = INT_MIN;
	bool onPlatform = false;
	bool testSlope = false;
	bool onSquareTile = false;

	bool lastFrameOnSlope = flag->bits & ON_SLOPE;
	flag->bits &= ~ON_SLOPE;

	if (centerFootT >= SMALL_SLOPE_LEFT1 && centerFootT <= LARGE_SLOPE_RIGHT)
	{
		testSlope = true;
		int32_t tileX = (tx->pos.x >> 12) << 12;
		int32_t tileY = ((tx->pos.y - 0x200) >> 12) << 12;
		tileH = GetGroundYForTileAndPos(tileX, tileY, centerFootT, tx->pos.x);
	}
	else if (centerFootT == SQUARE || (!lastFrameOnSlope && (leftFootT == SQUARE || rightFootT == SQUARE)))
	{
		onSquareTile = true;
		int tileY = (tx->pos.y - 0x200) >> 12;
		tileH = (tileY + 1) << 12;
	}

	int32_t platformH = INT_MIN;
	if (TestPoints[0].hitting && TestPoints[0].limit > tileH)
	{
		onPlatform = true;
		platformH = TestPoints[0].limit;
	}
	if (TestPoints[1].hitting && TestPoints[1].limit > tileH && TestPoints[1].limit > platformH)
	{
		onPlatform = true;
		platformH = TestPoints[1].limit;
	}
	if (TestPoints[2].hitting && TestPoints[2].limit > tileH && TestPoints[2].limit > platformH)
	{
		onPlatform = true;
		platformH = TestPoints[2].limit;
	}

	bool continuousSlope = false;
	if (onPlatform)
	{
		phys->vel.y = 0;
		tx->pos.y = platformH;
		flag->bits |= ON_GROUND;
	}
	else if (testSlope)
	{
		//if going from being on ground to being over a slope that is continuous with the ground, we will snap down to the slope
		continuousSlope = centerFootT == LARGE_SLOPE_RIGHT || centerFootT == LARGE_SLOPE_LEFT || centerFootT == SMALL_SLOPE_LEFT2 || centerFootT == SMALL_SLOPE_RIGHT2;
		if (!(flag->bits & HOLDING_JUMP) && ((phys->vel.y <= 0 && tx->pos.y <= tileH) || lastFrameOnSlope || (wasOnGround && continuousSlope)))
		{
			flag->bits |= ON_SLOPE;
			if (centerFootT == LARGE_SLOPE_LEFT || centerFootT == SMALL_SLOPE_LEFT2 || centerFootT == SMALL_SLOPE_LEFT1)
				flag->bits |= LEFT_SLOPE;
			else
				flag->bits &= ~LEFT_SLOPE;
			phys->vel.y = 0;
			tx->pos.y = tileH;
			flag->bits |= ON_GROUND;
		}
	}
	else if (onSquareTile)
	{
		phys->vel.y = 0;
		tx->pos.y = tileH;
		flag->bits |= ON_GROUND;
	}



	TestPoints[9].p = {tx->pos.x - info->headHalfWidth, tx->pos.y + info->headHeight};
	TestPoints[9].type = POINT_TOP;
	TestPoints[10].p = {tx->pos.x + info->headHalfWidth, tx->pos.y + info->headHeight};
	TestPoints[10].type = POINT_TOP;

	PlatformTest(goId, TestPoints + 9, 2);


	bool colHead = false;
	int colPointHead = INT_MAX;

	if (TileAtPoint(TestPoints[9].p) == SQUARE || TileAtPoint(TestPoints[10].p) == SQUARE)
	{
		colHead = true;
		int tileYatHead = (((tx->pos.y + info->headHeight) >> 12) << 12);
		colPointHead = tileYatHead - info->headHeight - (1 << 9);
	}

	if (TestPoints[9].hitting)
	{
		colHead = true;
		colPointHead = Min(TestPoints[9].limit - info->headHeight - (1 << 9), colPointHead);
	}
	if (TestPoints[10].hitting)
	{
		colHead = true;
		colPointHead = Min(TestPoints[10].limit - info->headHeight - (1 << 9), colPointHead);
	}

	if (phys->vel.y > 0)
	{
		if (colHead)
		{
			tx->pos.y = colPointHead;
			phys->vel.y = 0;
		}
	}

	int foot1 = 3;
	int foot2 = (((info->headHeight >> 9) - 3) / 2) + 3;
	int foot3 = info->headHeight >> 9;

	TestPoints[3].p = {tx->pos.x - info->bodyHalfWidth, tx->pos.y + (foot1 << 9)};
	TestPoints[3].type = POINT_LEFT;
	TestPoints[4].p = {tx->pos.x - info->bodyHalfWidth, tx->pos.y + (foot2 << 9)};
	TestPoints[4].type = POINT_LEFT;
	TestPoints[5].p = {tx->pos.x - info->bodyHalfWidth, tx->pos.y + (foot3 << 9)};
	TestPoints[5].type = POINT_LEFT;
	TestPoints[6].p = {tx->pos.x + info->bodyHalfWidth, tx->pos.y + (foot1 << 9)};
	TestPoints[6].type = POINT_RIGHT;
	TestPoints[7].p = {tx->pos.x + info->bodyHalfWidth, tx->pos.y + (foot2 << 9)};
	TestPoints[7].type = POINT_RIGHT;
	TestPoints[8].p = {tx->pos.x + info->bodyHalfWidth, tx->pos.y + (foot3 << 9)};
	TestPoints[8].type = POINT_RIGHT;

	PlatformTest(goId, TestPoints + 3, 6);


	bool colLeft = false;
	bool colRight = false;
	int32_t colPointL = INT_MIN;
	int32_t colPointR = INT_MAX;

	if (!(flag->bits & ON_LADDER))
	{
		if ((!continuousSlope && TileAtPoint(TestPoints[3].p) == SQUARE) || (TileAtPoint(TestPoints[4].p) == SQUARE) || (TileAtPoint(TestPoints[5].p) == SQUARE))
		{
			int tileX = (tx->pos.x - info->bodyHalfWidth) >> 12;
			colPointL = ((tileX + 1) << 12) + info->bodyHalfWidth;
			colLeft = true;
		}
		if ((!continuousSlope && TileAtPoint(TestPoints[6].p) == SQUARE) || (TileAtPoint(TestPoints[7].p) == SQUARE) || (TileAtPoint(TestPoints[8].p) == SQUARE))
		{
			int tileX = (tx->pos.x + info->bodyHalfWidth) >> 12;
			colPointR = ((tileX) << 12) - info->bodyHalfWidth;
			colRight = true;
		}
	}

	if (TestPoints[3].hitting)
	{
		colLeft = true;
		colPointL = Max(TestPoints[3].limit + info->bodyHalfWidth, colPointL);
	}
	if (TestPoints[4].hitting)
	{
		colLeft = true;
		colPointL = Max(TestPoints[4].limit + info->bodyHalfWidth, colPointL);
	}
	if (TestPoints[5].hitting)
	{
		colLeft = true;
		colPointL = Max(TestPoints[5].limit + info->bodyHalfWidth, colPointL);
	}

	if (TestPoints[6].hitting)
	{
		colRight = true;
		colPointR = Min(TestPoints[6].limit - info->bodyHalfWidth, colPointR);
	}
	if (TestPoints[7].hitting)
	{
		colRight = true;
		colPointR = Min(TestPoints[7].limit - info->bodyHalfWidth, colPointR);
	}
	if (TestPoints[8].hitting)
	{
		colRight = true;
		colPointR = Min(TestPoints[8].limit - info->bodyHalfWidth, colPointR);
	}

	if (colLeft)
	{
		if (phys->vel.x <= 0)
			phys->vel.x = 0;
		tx->pos.x = colPointL;
	}

	if (colRight)
	{
		if (phys->vel.x >= 0)
			phys->vel.x = 0;
		tx->pos.x = colPointR;
	}

	//Check if standing on slope
	uint8_t footT = TileAtPoint({tx->pos.x, tx->pos.y});
	if (footT >= SMALL_SLOPE_LEFT1 && footT <= LARGE_SLOPE_RIGHT)
	{
		int32_t tileX = (tx->pos.x >> 12) << 12;
		int32_t tileY = ((tx->pos.y) >> 12) << 12;
		int32_t groundY = GetGroundYForTileAndPos(tileX, tileY, footT, tx->pos.x);
		if (phys->vel.y <= 0 && tx->pos.y <= groundY)
		{
			flag->bits |= ON_SLOPE;
			if (footT == LARGE_SLOPE_LEFT || footT == SMALL_SLOPE_LEFT2 || footT == SMALL_SLOPE_LEFT1)
				flag->bits |= LEFT_SLOPE;
			else
				flag->bits &= ~LEFT_SLOPE;
			phys->vel.y = 0;
			tx->pos.y = groundY;
			flag->bits |= ON_GROUND;
		}
	}

	if (flag->bits & ON_GROUND)
	{
		phys->accel.y = 0;
	}
	else
	{
		phys->accel.y = info->gravity;
	}

	int horzImpSign = Sign(phys->accel.x);
	int maxVelocity = info->maxVelocity;

	int horizontalDamping = info->horizontalDamping;
	if (!(flag->bits & ON_GROUND))
	{
		horizontalDamping = info->airborneHorizontalDamping;
	}

	float newXScale = tx->scale.x;

	if (phys->vel.x > 0)
	{
		if (flag->bits & ON_GROUND)
		{
			newXScale = 1;
			if (phys->vel.x > maxVelocity)
				phys->vel.x = maxVelocity;
		}
		else
		{
			float scaleX = Sign(phys->accel.x); 
			if (scaleX != 0)
				newXScale = scaleX;
		}
		if (phys->vel.x > horizontalDamping)
			phys->vel.x -= horizontalDamping;
		else
			phys->vel.x = 0;
	}
	else if (phys->vel.x < 0)
	{
		if (flag->bits & ON_GROUND)
		{
			newXScale = -1;
			if (phys->vel.x < -maxVelocity)
				phys->vel.x = -maxVelocity;
		}
		else
		{
			float scaleX = Sign(phys->accel.x); 
			if (scaleX != 0)
				newXScale = scaleX;
		}
		if (phys->vel.x < -horizontalDamping)
			phys->vel.x += horizontalDamping;
		else
			phys->vel.x = 0;
	}
	else
	{
		if (horzImpSign)
			newXScale = horzImpSign;
	}

	if (info->setXScale)
		tx->scale.x = newXScale;

	rider->size.y = (foot3 << 9);

}


enum health_bar_counters
{
	HEALTH_BAR_PLAYER_HEALTH	= 0,
};

UPDATE_FUNCTION(HealthBarDraw)
{
	auto counters = GO(counters);
	SetShader("basic");
	float t = App.gameWindowSize.y << 1;
	float b = t - 16;
	float l = 0;
	float r = App.gameWindowSize.x << 1;
	DrawQuad(MAKE_COLOR(0,0,0,255), {l, t}, {r, t}, {r, b}, {l, b});
	SetShader("basic_tex_color");
	SetFont("PixelOperator", 15);
	SetFontColor(MAKE_COLOR(255,100,0,255));
	DrawText(2, b + 3, "Health");

	uint8_t health = GetCounter(counters->counters, HEALTH_BAR_PLAYER_HEALTH);
	for (int i = 0; i < health; i++)
		DrawSprite("HeartFull", COL_WHITE, {l + 50 + i*12, b + 2}, 0, {1,1});
	for (int i = health; i < 5; i++)
		DrawSprite("HeartEmpty", COL_WHITE, {l + 50 + i*12, b + 2}, 0, {1,1});
}

int CreateHealthBar()
{
	int goId = AddObject(OBJ_HEALTH_BAR);
	auto meta = GO(metadata);
	auto drawgui = GO(draw_game_gui);
	meta->cmpInUse = COUNTERS | DRAW_GAME_GUI;
	meta->flags = GAME_OBJECT;
	InitObject(goId);
	drawgui->draw = HealthBarDraw;
	return goId;
}


enum game_freeze_counters
{
	GAME_FREEZE_NUM_FRAMES		= 0,
	GAME_FREEZE_CUR_FRAME		= 1,
	GAME_FREEZE_DELAY			= 2,
};

UPDATE_FUNCTION(GameFreezeUpdate)
{
	auto counters = GO(counters);
	if (DecrementCounter(&counters->counters, GAME_FREEZE_DELAY) == 0)
	{
		SendingGameUpdateEvents = false;
		if (IncrementCounter(&counters->counters, GAME_FREEZE_CUR_FRAME) > GetCounter(counters->counters, GAME_FREEZE_NUM_FRAMES))
		{
			RemoveObject(goId);
			SendingGameUpdateEvents = true;
		}
	}
}

void CreateGameFreezer(int delay, int frames)
{
	int goId = AddObject(OBJ_SWORD_SWIPE);
	auto meta = GO(metadata);
	auto counters = GO(counters);
	auto update = GO(update);
	meta->cmpInUse = COUNTERS | UPDATE;
	InitObject(goId);
	SetCounter(&counters->counters, GAME_FREEZE_NUM_FRAMES, frames);
	SetCounter(&counters->counters, GAME_FREEZE_DELAY, delay);
	update->update = GameFreezeUpdate;
}

enum sword_flags
{
	SWORD_INITIALIZED		= 0x1,
	SWORD_COLLISION_TESTED	= 0x2,

	SWIPE_UP				= 0x4,
	SWIPE_DOWN				= 0x8,
	SWIPE_LEFT				= 0x10,
	SWIPE_RIGHT				= 0x20,
};

UPDATE_FUNCTION(SwordSwipeUpdate)
{
	auto tx = GO(transform);
	auto anim = GO(anim);
	auto parent = GO(parent);
	auto flag = GO(custom_flags);
	auto collider = GO(collider);
	tx->pos = OTH(parent->parentID, transform)->pos;


	if (!(flag->bits & SWORD_INITIALIZED) && !(flag->bits & SWORD_COLLISION_TESTED))
	{
		flag->bits |= SWORD_INITIALIZED;
		collider->mask = 0xffff;
	}
	else if (!(flag->bits & SWORD_COLLISION_TESTED))
	{
		flag->bits |= SWORD_COLLISION_TESTED;
		collider->mask = 0;
	}

	if (!anim->playing)
	{
		RemoveObject(goId);
	}
}

void CreateSwordSwipe(int parentID, ivec2 pos, uint32_t dir)
{
	int goId = AddObject(OBJ_SWORD_SWIPE);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto anim = GO(anim);
	auto collider = GO(collider);
	auto update = GO(update);
	auto parent = GO(parent);
	auto flag = GO(custom_flags);
	meta->cmpInUse = CUSTOM_FLAGS | UPDATE | PARENT | TRANSFORM | COLLIDER | SPRITE | ANIM;
	meta->flags |= GAME_OBJECT;
	InitObject(goId);
	tx->pos = pos;
	anim->loop = false;
	anim->playing = true;
	anim->frameTime = 0;
	update->update = SwordSwipeUpdate;
	flag->bits |= dir;

	if (flag->bits & SWIPE_RIGHT)
	{
		collider->ur = ivec2((40 << 9), (12 << 9));
		collider->bl = ivec2(0, -(8 << 9));
		tx->scale.x = 1;
		anim->name = "sword_side";
	}
	else if (flag->bits & SWIPE_LEFT)
	{
		collider->ur = ivec2(0, (12 << 9));
		collider->bl = ivec2(-(40 << 9), -(8 << 9));
		tx->scale.x = -1;
		anim->name = "sword_side";
	} 
	else if (flag->bits & SWIPE_UP)
	{
		collider->ur = ivec2((10 << 9), (40 << 9));
		collider->bl = ivec2(-(10 << 9), 0);
		tx->scale.x = 1;
		anim->name = "sword_up";
	} 
	else if (flag->bits & SWIPE_DOWN)
	{
		collider->ur = ivec2((10 << 9), 0);
		collider->bl = ivec2(-(10 << 9), -(40 << 9));
		tx->scale.x = 1;
		anim->name = "sword_down";
	} 

	collider->mask = 0;
	parent->parentID = parentID;
}

UPDATE_FUNCTION(DeleteOnAnimFinishUpdate)
{
	auto anim = GO(anim);
	if (anim->playing == false)
	{
		RemoveObject(goId);
	}
}

enum particle_type
{
	DUST_SYMMETRICAL_PUFF,
	DUST_SINGLE_SIDED_PUFF,
	DUST_RUNNING_PUFF,
	BLOOD_SPLATTER_SIDE,
	BLOOD_SPLATTER_UP,
};

void CreateAnimatedParticles(ivec2 pos, particle_type type, bool puffRight, float rotation)
{
	int goId = AddObject(OBJ_DUST);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto anim = GO(anim);
	auto update = GO(update);
	auto sprite = GO(sprite);
	meta->cmpInUse = UPDATE | TRANSFORM | SPRITE | ANIM;
	meta->flags |= GAME_OBJECT;
	InitObject(goId);
	tx->pos = pos;
	if (!puffRight)
		tx->scale.x = -1;
	tx->rot = rotation;

	switch (type)
	{
		case DUST_SYMMETRICAL_PUFF:
		{
			anim->name = "landing_dust";
		} break;
		case DUST_SINGLE_SIDED_PUFF:
		{
			anim->name = "landing_dust_one_side";
		} break;
		case DUST_RUNNING_PUFF:
		{
			anim->name = "running_dust";
			float scale = (rand() % 5) / 10.0f + 0.5f;
			tx->scale = scale * tx->scale;
		} break;
		case BLOOD_SPLATTER_SIDE:
		{
			anim->name = "blood_splatter_side";
		} break;
		case BLOOD_SPLATTER_UP:
		{
			anim->name = "blood_splatter_up";
		} break;
	}

	anim->loop = false;
	anim->playing = true;
	anim->frameTime = 0;
	sprite->depth = 8;
	update->update = DeleteOnAnimFinishUpdate;
}


UPDATE_FUNCTION(KnockBackUpdate)
{
	auto counters = GO(counters);
	//stick around for a frame
	if (IncrementCounter(&counters->counters, 0) > 1)
		RemoveObject(goId);
}

void CreateKnockBack(ivec2 pos, ivec2 amount)
{
	int goId = AddObject(OBJ_KNOCKBACK);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto phys = GO(physics);
	auto update = GO(update);
	auto collider = GO(collider);
	meta->cmpInUse = UPDATE | PHYSICS | TRANSFORM | COLLIDER;
	meta->flags |= GAME_OBJECT;
	InitObject(goId);
	tx->pos = pos;
	phys->vel = amount;
	update->update = KnockBackUpdate;
	collider->ur = ivec2(5<<9,5<<9);
	collider->bl = ivec2(-(5<<9),0);
}

int GetLevelIDFromLevelName(char *levelName)
{
	int result = -1;
	int curLevel = 0;

	void *value;
	size_t iter = HashGetFirst(&Levels, NULL, NULL, &value);
	while (value)
	{
		json_data_file *file = (json_data_file *)value;
		if (strcmp(levelName, file->fileName) == 0)
		{
			result = curLevel;
			break;
		}
		curLevel++;
		HashGetNext(&Levels, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter); 

	return result;
}

UPDATE_FUNCTION(PlayerUpdate)
{
	auto phys = GO(physics);
	auto tx = GO(transform);
	auto flag = GO(custom_flags);
	auto anim = GO(anim);
	auto counters = GO(counters);
	auto collider = GO(collider);
	auto rider = GO(rides_platforms);

	int healthBarID = GetFirstOfType(OBJ_HEALTH_BAR);
	if (healthBarID == -1)
	{
		healthBarID = CreateHealthBar();
		SetCounter(&(OTH(healthBarID, counters)->counters), HEALTH_BAR_PLAYER_HEALTH, 5);
	}

	bool knockBack = DecrementCounter(&counters->counters2, PLYR_KNOCKBACK) > 0;


	if (KeyboardPresses[KB_K])
	{
		CreateKnockBack(tx->pos, ivec2(0x500,0));
	}


	bool hasControl = !(flag->bits & CONTROL_LOCKED);

	bool keyUpPressed = hasControl && KeyPresses[KEY_Up];
	bool keyDownPressed = hasControl && KeyPresses[KEY_Down];
	bool keyLeftPressed = hasControl && !knockBack && KeyPresses[KEY_Left];
	bool keyRightPressed = hasControl && !knockBack && KeyPresses[KEY_Right];
	bool keyAction1Pressed = hasControl && KeyPresses[KEY_Action1];
	bool keyAction2Pressed = hasControl && KeyPresses[KEY_Action2];

	bool keyUpDown = hasControl && KeyDown[KEY_Up];
	bool keyDownDown = hasControl && KeyDown[KEY_Down];
	bool keyLeftDown = hasControl && !knockBack && KeyDown[KEY_Left];
	bool keyRightDown = hasControl && !knockBack && KeyDown[KEY_Right];
	bool keyAction1Down = hasControl && KeyDown[KEY_Action1];
	bool keyAction2Down = hasControl && KeyDown[KEY_Action2];


	if (keyAction2Pressed && !(flag->bits & ON_LADDER))
	{
		if (keyDownDown && !(flag->bits & ON_GROUND))
			CreateSwordSwipe(goId, tx->pos, SWIPE_DOWN);
		else if (keyUpDown)
			CreateSwordSwipe(goId, tx->pos, SWIPE_UP);
		else if (keyLeftDown)
			CreateSwordSwipe(goId, tx->pos + ivec2(0, 6 << 9), SWIPE_LEFT);
		else if (keyRightDown)
			CreateSwordSwipe(goId, tx->pos + ivec2(0, 6 << 9), SWIPE_RIGHT);
		else
			CreateSwordSwipe(goId, tx->pos + ivec2(0, 6 << 9), Sign(tx->scale.x) > 0 ? SWIPE_RIGHT : SWIPE_LEFT);
	}

	if (hasControl && KeyboardPresses[KB_D])
	{
		CreateDialog({200<<9,100<<9}, tx->pos, "Show some motha fuckin dialog"); 
	}

	local_persistent int oldCameraW;
	local_persistent int oldCameraH;
	local_persistent bool resetAfterHit = false;
	local_persistent char *animName;

	if (resetAfterHit)
	{
		resetAfterHit = false;
		Camera.width = oldCameraW;
		Camera.height = oldCameraH;
		anim->name = animName;
	}

	//
	if (GetCounter(counters->counters2, PLYR_INVICIBLE) > 0 && DecrementCounter(&counters->counters2, PLYR_INVICIBLE) == 0)
		collider->mask = 0xffff;

	for (int i = 0; i < ArrayCount(collider->collisions); i++)
	{
		int othID = collider->collisions[i];
		if (othID != -1)
		{
			object_type type = GameComponents.type[GameObjectIDs[othID].index];
			switch (type)
			{
				case OBJ_DOOR:
				{
					if (keyUpDown)
					{
						auto str = OTH(othID, string_storage);
						if (str->string)
						{
							char levelName[64];
							strsplitindex(str->string, '#', 0, levelName);

							int levelID = GetLevelIDFromLevelName(levelName);
							if (levelID > -1)
							{
								OnLevel = levelID;
								LevelLoaderInitialized = false;
							}
						}
					}
				} break;

				case OBJ_ENEMY_PROJECTILE:
				case OBJ_ENEMY_JUMPER:
				case OBJ_ENEMY_SWORD_SWIPE:
				case OBJ_ENEMY_SWOOPER:
				case OBJ_STANDIN:
				{
					CreateGameFreezer(0, 20);

					oldCameraW = Camera.width;
					oldCameraH = Camera.height;
					animName = anim->name;
					anim->name = "girl_getting_hit";
					AnimationUpdate(goId);
					Camera.width *= 0.9f;
					Camera.height *= 0.9f;
					resetAfterHit = true;

					DecrementCounter(&(OTH(healthBarID, counters)->counters), HEALTH_BAR_PLAYER_HEALTH);
					collider->mask = 0;
					SetCounter(&counters->counters2, PLYR_INVICIBLE, 60);

				} break;


				case OBJ_KNOCKBACK:
				{
					phys->vel = OTH(othID, physics)->vel;
					SetCounter(&counters->counters2, PLYR_KNOCKBACK, 30);
				} break;
				/*
				case OBJ_CRYSTAL_GENERATOR:
				{
					if (hasControl && KeyPresses[KEY_Down])
						CreateGeneratorDisplay();
				} break;

				case OBJ_CRYSTAL:
				{
					if (hasControl && KeyPresses[KEY_Action2])
					{
						uint8_t carrying = GetCounter(counters->counters, PLYR_ITEM_CARRIED);
						if (!(carrying & 0xf0)) //already carrying
						{
							auto othcounters = OTH(othID, counters);
							uint8_t crystalVal = GetCounter(othcounters->counters, 0);
							RemoveObject(othID);
							SetCounter(&counters->counters, PLYR_ITEM_CARRIED, 0x10 | crystalVal);
							KeyPresses[KEY_Action2] = false;
						}
					}
				} break;
				*/
				default: {} break;
			}
		}
		else
			break;
	}

	/*
	if (hasControl && KeyPresses[KEY_Action2])
	{
		uint8_t carrying = GetCounter(counters->counters, PLYR_ITEM_CARRIED);
		switch (carrying & 0xf0)
		{
			case 0x10: //crystal
			{
				ivec2 crystalVel = phys->vel + ivec2(((int)Sign(tx->scale.x))<<9,0x150);
				if (KeyDown[KEY_Up])
					crystalVel = phys->vel + ivec2(0, 0x400);
				else if (KeyDown[KEY_Down])
					crystalVel = phys->vel + ivec2(0, -0x100);
				CreatePowerCrystal(tx->pos + ivec2(0,12<<9), carrying & 0xf, crystalVel);
				SetCounter(&counters->counters, PLYR_ITEM_CARRIED, 0);
			} break;
		}
	}
	*/
	
	/*
	if (KeyboardPresses[KB_D])
	{
		action pauseGame = {};
		pauseGame.type = ACTION_Pause_game;

		action removeControl = {};
		removeControl.type = ACTION_Remove_character_control;

		action waitASec = {};
		waitASec.type = ACTION_Wait;
		waitASec.wait.timeToWait = 0.8f;

		action setAnimation = {};
		setAnimation.type = ACTION_Set_animation;
		setAnimation.set_animation.objectID = goId;
		setAnimation.set_animation.animation = "girl_running";

		action setSpeed = {};
		setSpeed.type = ACTION_Set_speed;
		setSpeed.set_speed.objectID = goId;
		setSpeed.set_speed.speed.x = 0x150;
		setSpeed.set_speed.setX = true;
		setSpeed.set_speed.stopOnFinish = true;
		setSpeed.set_speed.setToPosOnFinish = true;
		setSpeed.set_speed.greaterThan = true;
		setSpeed.set_speed.usesFinishX = true;
		setSpeed.set_speed.finishX = 4200 << 9;
		setSpeed.set_speed.usesFinishY = false;

		action showDialog = {};
		showDialog.type = ACTION_Show_dialogue_box;

		action addLine = {};
		addLine.type = ACTION_Add_dialogue_string;
		addLine.add_dialogue_string.string = "This is working so far...";

		action waitForKey = {};
		waitForKey.type = ACTION_Wait_for_dialogue_advance;

		action removeDialog = {};
		removeDialog.type = ACTION_Remove_dialogue_box;

		action returnControl = {};
		returnControl.type = ACTION_Return_character_control;

		action resumeGame = {};
		resumeGame.type = ACTION_Resume_game;


		action *a = AddStartingAction(removeControl);
		AddSubsequentAction(&a, waitASec);
		AddSubsequentAction(&a, setAnimation);
		AddSubsequentAction(&a, setSpeed);
		AddSubsequentAction(&a, pauseGame);
		AddSubsequentAction(&a, showDialog);
		AddSubsequentAction(&a, addLine);
		AddSubsequentAction(&a, waitForKey);
		addLine.add_dialogue_string.string = "Ok, that worked too....";
		AddSubsequentAction(&a, addLine);
		AddSubsequentAction(&a, waitForKey);
		addLine.add_dialogue_string.string = "Now you will feel the might of yada yada yada.";
		AddSubsequentAction(&a, addLine);
		AddSubsequentAction(&a, waitForKey);
		AddSubsequentAction(&a, removeDialog);
		AddSubsequentAction(&a, returnControl);
		AddSubsequentAction(&a, resumeGame);
	}
	*/

	/*
	IncrementCounterNoLoop(&counters->counters, PLYR_TIME_SINCE_FIRED);
	if (hasControl && KeyDown[KEY_Action2])
	{
		if (strcmp(anim->name, "girl_jumping") == 0)
			anim->name = "girl_jumping_firing_wand";

		if (GetCounter(counters->counters, PLYR_NEXT_SHOT) == 0)
		{
			CreateMagicBullet(ivec2(tx->pos.x + ((8 << 9) * Sign(tx->scale.x)), tx->pos.y + (8 << 9)), (0x450 * ivec2(Sign(tx->scale.x), 0)));
			SetCounter(&counters->counters, PLYR_NEXT_SHOT, rand()%6 + 9);
			SetCounter(&counters->counters, PLYR_TIME_SINCE_FIRED, 0);
		}

		if (GetCounter(counters->counters, PLYR_NEXT_SHOT) == 0)
		{
			CreateBasicBullet(ivec2(tx->pos.x + ((8 << 9) * Sign(tx->scale.x)), tx->pos.y + (9 << 9)), (0x600 * ivec2(Sign(tx->scale.x), 0)));
			SetCounter(&counters->counters, PLYR_NEXT_SHOT, 8);
			SetCounter(&counters->counters, PLYR_TIME_SINCE_FIRED, 0);
		}
	}
	DecrementCounter(&counters->counters, PLYR_NEXT_SHOT);
	*/

	tile_collision_info info;
	info.maxFallSpeed = -0x800;
	info.headHalfWidth = 4 << 9;
	if (flag->bits & ON_LADDER)
		info.headHalfWidth = 3 << 9;
	info.headHeight = 15 << 9;
	if ((flag->bits & SLIDING) || (flag->bits & SKIDDING))
		info.headHeight = 8 << 9;

	info.bodyHalfWidth = 6 << 9;
	if (flag->bits & HOLDING_JUMP)
		info.gravity = -22;
	else
		info.gravity = -36;

	int maxSlidingVelocity = 0x460;
	int maxRunningVelocity = 0x280;
	info.maxVelocity = ((flag->bits & SLIDING) || (flag->bits & SKIDDING)) ? maxSlidingVelocity : maxRunningVelocity;
	info.horizontalDamping = 12;
	info.airborneHorizontalDamping = 2;

	if (knockBack)
	{
		info.maxVelocity = 0x1000;
		info.horizontalDamping = 8;
	}


	//check for crush condition
	bool crushed = false;
	if (rider->pushedV)
	{
		if (rider->pushedV < 0 && ((flag->bits & ON_GROUND) || (flag->bits & ON_SLOPE)))
			crushed = true;
		rider->pushedV = 0;
	}
	if (rider->pushedH)
	{
		rider->pushedH = 0;
	}

	phys->vel.y = Max(info.maxFallSpeed, phys->vel.y);

	int oldYVel = phys->vel.y;
	bool wasOnGround = flag->bits & ON_GROUND;
	flag->bits &= ~ON_GROUND;
	IncrementCounterNoLoop(&counters->counters, PLYR_SINCE_LEFT_GROUND);

	test_point TestPoints[11] = {};
	TestPoints[0].p = {tx->pos.x, tx->pos.y - (1 << 9)};
	TestPoints[0].type = POINT_BOTTOM;
	TestPoints[1].p = {tx->pos.x - (3 << 9), tx->pos.y - (1 << 9)};
	TestPoints[1].type = POINT_BOTTOM;
	TestPoints[2].p = {tx->pos.x + (3 << 9), tx->pos.y - (1 << 9)};
	TestPoints[2].type = POINT_BOTTOM;

	PlatformTest(goId, TestPoints + 0, 3);

	uint8_t centerFootT = TileAtPoint(TestPoints[0].p, tx->oldY);
	uint8_t leftFootT = TileAtPoint(TestPoints[1].p, tx->oldY);
	uint8_t rightFootT = TileAtPoint(TestPoints[2].p, tx->oldY);

	tx->oldY = tx->pos.y;

	int32_t tileH = INT_MIN;
	bool onPlatform = false;
	bool testSlope = false;
	bool onSquareTile = false;

	bool lastFrameOnSlope = flag->bits & ON_SLOPE;
	flag->bits &= ~ON_SLOPE;

	if (centerFootT >= SMALL_SLOPE_LEFT1 && centerFootT <= LARGE_SLOPE_RIGHT)
	{
		testSlope = true;
		int32_t tileX = (tx->pos.x >> 12) << 12;
		int32_t tileY = ((tx->pos.y - 0x200) >> 12) << 12;
		tileH = GetGroundYForTileAndPos(tileX, tileY, centerFootT, tx->pos.x);
	}
	else if (centerFootT == SQUARE || (!lastFrameOnSlope && (leftFootT == SQUARE || rightFootT == SQUARE)))
	{
		onSquareTile = true;
		int tileY = (tx->pos.y - 0x200) >> 12;
		tileH = (tileY + 1) << 12;
	}

	int32_t platformH = INT_MIN;
	if (TestPoints[0].hitting && TestPoints[0].limit > tileH)
	{
		onPlatform = true;
		platformH = TestPoints[0].limit;
	}
	if (TestPoints[1].hitting && TestPoints[1].limit > tileH && TestPoints[1].limit > platformH)
	{
		onPlatform = true;
		platformH = TestPoints[1].limit;
	}
	if (TestPoints[2].hitting && TestPoints[2].limit > tileH && TestPoints[2].limit > platformH)
	{
		onPlatform = true;
		platformH = TestPoints[2].limit;
	}

	bool continuousSlope = false;
	if (onPlatform)
	{
		phys->vel.y = 0;
		tx->pos.y = platformH;
		SetCounter(&counters->counters, PLYR_SINCE_LEFT_GROUND, 0);
		flag->bits |= ON_GROUND;
		flag->bits &= ~ON_LADDER;
	}
	else if (testSlope)
	{
		//if going from being on ground to being over a slope that is continuous with the ground, we will snap down to the slope
		continuousSlope = centerFootT == LARGE_SLOPE_RIGHT || centerFootT == LARGE_SLOPE_LEFT || centerFootT == SMALL_SLOPE_LEFT2 || centerFootT == SMALL_SLOPE_RIGHT2;
		if (!(flag->bits & HOLDING_JUMP) && ((phys->vel.y <= 0 && tx->pos.y <= tileH) || lastFrameOnSlope || (wasOnGround && continuousSlope)))
		{
			flag->bits |= ON_SLOPE;
			if (centerFootT == LARGE_SLOPE_LEFT || centerFootT == SMALL_SLOPE_LEFT2 || centerFootT == SMALL_SLOPE_LEFT1)
				flag->bits |= LEFT_SLOPE;
			else
				flag->bits &= ~LEFT_SLOPE;
			phys->vel.y = 0;
			tx->pos.y = tileH;
			SetCounter(&counters->counters, PLYR_SINCE_LEFT_GROUND, 0);
			flag->bits |= ON_GROUND;
			flag->bits &= ~ON_LADDER;
		}
	}
	else if (onSquareTile)
	{
		phys->vel.y = 0;
		tx->pos.y = tileH;
		SetCounter(&counters->counters, PLYR_SINCE_LEFT_GROUND, 0);
		flag->bits |= ON_GROUND;
		flag->bits &= ~ON_LADDER;
	}



	uint8_t centerFootUpperT = TileAtPoint({tx->pos.x, tx->pos.y + (9 << 9)}, tx->oldY);

	if (!(flag->bits & ON_LADDER) && centerFootUpperT == LADDER && keyUpDown && GetCounter(counters->counters, PLYR_TIME_SINCE_JUMP) > 15)
	{
		if (flag->bits & ON_GROUND)
			tx->pos.y += (1<<9); //get off the ground, otherwise character gets stuck
		flag->bits |= ON_LADDER;
		flag->bits &= ~ON_GROUND;
		anim->name = "girl_climbing_ladder";
		anim->loop = true;
		anim->playing = false;
		anim->frameTime = 0;
	}

	uint8_t centerFootBelowT = TileAtPoint({tx->pos.x, tx->pos.y - (1 << 9)});

	if (centerFootBelowT == LADDER_TOP && keyDownDown)
	{
		tx->pos.y -= (1<<9);
		flag->bits |= ON_LADDER;
		flag->bits &= ~ON_GROUND;
		anim->name = "girl_climbing_ladder";
		anim->loop = true;
		anim->playing = false;
		anim->frameTime = 0;
	}

	if (flag->bits & ON_LADDER)
	{
		flag->bits &= ~HOLDING_JUMP;
		phys->vel.y = 0;
		phys->vel.x = 0;
		phys->accel.x = 0;
		phys->accel.y = 0;

		int32_t tileX = ((tx->pos.x >> 12) << 12) + (4 << 9);
		tx->pos.x = tileX;

		if (keyUpDown)
			phys->vel.y = 0x100;
		else if (keyDownDown)
			phys->vel.y = -0x100;

		anim->playing = (phys->vel.y != 0);

		uint8_t centerFootLowerT = TileAtPoint({tx->pos.x, tx->pos.y}, tx->oldY);
		if (centerFootUpperT != LADDER && centerFootLowerT != LADDER)
			flag->bits &= ~ON_LADDER;

		if (keyAction1Pressed)
		{
			if (keyLeftDown)
				phys->vel.x = -0x180;
			else if (keyRightDown)
				phys->vel.x = 0x180;

			phys->vel.y = 0x310;
			flag->bits &= ~ON_LADDER;
			anim->name = "girl_jumping";
			anim->frameTime = 0;
			anim->loop = false;
			anim->playing = true;
			
			SetCounter(&counters->counters, PLYR_TIME_SINCE_JUMP, 0);
		}
	}
	IncrementCounterNoLoop(&counters->counters, PLYR_TIME_SINCE_JUMP);


	TestPoints[9].p = {tx->pos.x - info.headHalfWidth, tx->pos.y + info.headHeight};
	TestPoints[9].type = POINT_TOP;
	TestPoints[10].p = {tx->pos.x + info.headHalfWidth, tx->pos.y + info.headHeight};
	TestPoints[10].type = POINT_TOP;

	PlatformTest(goId, TestPoints + 9, 2);


	bool colHead = false;
	int colPointHead = INT_MAX;

	if (TileAtPoint(TestPoints[9].p) == SQUARE || TileAtPoint(TestPoints[10].p) == SQUARE)
	{
		colHead = true;
		int tileYatHead = (((tx->pos.y + info.headHeight) >> 12) << 12);
		colPointHead = tileYatHead - info.headHeight - (1 << 9);
	}

	if (TestPoints[9].hitting)
	{
		colHead = true;
		colPointHead = Min(TestPoints[9].limit - info.headHeight - (1 << 9), colPointHead);
	}
	if (TestPoints[10].hitting)
	{
		colHead = true;
		colPointHead = Min(TestPoints[10].limit - info.headHeight - (1 << 9), colPointHead);
	}

	if (phys->vel.y > 0)
	{
		if (colHead)
		{
			tx->pos.y = colPointHead;
			phys->vel.y = 0;
		}
	}


	int foot1 = 3;
	int foot2 = (((info.headHeight >> 9) - 3) / 2) + 3;
	int foot3 = (info.headHeight >> 9);

	collider->ur.y = info.headHeight;
	if ((flag->bits & SLIDING) || (flag->bits & SKIDDING))
	{
		foot2 = foot3 = 8;
	}

	TestPoints[3].p = {tx->pos.x - info.bodyHalfWidth, tx->pos.y + (foot1 << 9)};
	TestPoints[3].type = POINT_LEFT;
	TestPoints[4].p = {tx->pos.x - info.bodyHalfWidth, tx->pos.y + (foot2 << 9)};
	TestPoints[4].type = POINT_LEFT;
	TestPoints[5].p = {tx->pos.x - info.bodyHalfWidth, tx->pos.y + (foot3 << 9)};
	TestPoints[5].type = POINT_LEFT;
	TestPoints[6].p = {tx->pos.x + info.bodyHalfWidth, tx->pos.y + (foot1 << 9)};
	TestPoints[6].type = POINT_RIGHT;
	TestPoints[7].p = {tx->pos.x + info.bodyHalfWidth, tx->pos.y + (foot2 << 9)};
	TestPoints[7].type = POINT_RIGHT;
	TestPoints[8].p = {tx->pos.x + info.bodyHalfWidth, tx->pos.y + (foot3 << 9)};
	TestPoints[8].type = POINT_RIGHT;

	PlatformTest(goId, TestPoints + 3, 6);


	bool colLeft = false;
	bool colRight = false;
	int32_t colPointL = INT_MIN;
	int32_t colPointR = INT_MAX;

	if (!(flag->bits & ON_LADDER))
	{
		if ((!continuousSlope && TileAtPoint(TestPoints[3].p) == SQUARE) || (TileAtPoint(TestPoints[4].p) == SQUARE) || (TileAtPoint(TestPoints[5].p) == SQUARE))
		{
			int tileX = (tx->pos.x - info.bodyHalfWidth) >> 12;
			colPointL = ((tileX + 1) << 12) + info.bodyHalfWidth;
			colLeft = true;
		}
		if ((!continuousSlope && TileAtPoint(TestPoints[6].p) == SQUARE) || (TileAtPoint(TestPoints[7].p) == SQUARE) || (TileAtPoint(TestPoints[8].p) == SQUARE))
		{
			int tileX = (tx->pos.x + info.bodyHalfWidth) >> 12;
			colPointR = ((tileX) << 12) - info.bodyHalfWidth;
			colRight = true;
		}
	}

	if (TestPoints[3].hitting)
	{
		colLeft = true;
		colPointL = Max(TestPoints[3].limit + info.bodyHalfWidth, colPointL);
	}
	if (TestPoints[4].hitting)
	{
		colLeft = true;
		colPointL = Max(TestPoints[4].limit + info.bodyHalfWidth, colPointL);
	}
	if (TestPoints[5].hitting)
	{
		colLeft = true;
		colPointL = Max(TestPoints[5].limit + info.bodyHalfWidth, colPointL);
	}

	if (TestPoints[6].hitting)
	{
		colRight = true;
		colPointR = Min(TestPoints[6].limit - info.bodyHalfWidth, colPointR);
	}
	if (TestPoints[7].hitting)
	{
		colRight = true;
		colPointR = Min(TestPoints[7].limit - info.bodyHalfWidth, colPointR);
	}
	if (TestPoints[8].hitting)
	{
		colRight = true;
		colPointR = Min(TestPoints[8].limit - info.bodyHalfWidth, colPointR);
	}

	if (colLeft)
	{
		if (phys->vel.x <= 0)
			phys->vel.x = 0;
		tx->pos.x = colPointL;
		flag->bits &= ~ON_LADDER;
	}

	if (colRight)
	{
		if (phys->vel.x >= 0)
			phys->vel.x = 0;
		tx->pos.x = colPointR;
		flag->bits &= ~ON_LADDER;
	}

	TestPoints[3].p = {tx->pos.x - info.bodyHalfWidth - (1 << 9), tx->pos.y + (foot1 << 9)};
	TestPoints[3].type = POINT_LEFT;
	PlatformTest(goId, TestPoints + 3, 1);
	TestPoints[6].p = {tx->pos.x + info.bodyHalfWidth + (1 << 9), tx->pos.y + (foot1 << 9)};
	TestPoints[6].type = POINT_RIGHT;
	PlatformTest(goId, TestPoints + 6, 1);

	//test if it is possible to wall jump
	if ((TileAtPoint(TestPoints[3].p) == SQUARE) || TestPoints[3].hitting)
		flag->bits |= WALL_ON_LEFT;
	else
		flag->bits &= ~WALL_ON_LEFT;
	
	if ((TileAtPoint(TestPoints[6].p) == SQUARE) || TestPoints[6].hitting)
		flag->bits |= WALL_ON_RIGHT;
	else
		flag->bits &= ~WALL_ON_RIGHT;

	//reset our ability to wall jump if we land on the ground
	if (flag->bits & ON_GROUND)
	{
		flag->bits &= ~USED_WALL_JUMP;
		flag->bits &= ~ON_WALL;
		flag->bits &= ~WALL_JUMPING;
	}

	if (!(flag->bits & ON_GROUND) && !(flag->bits & USED_WALL_JUMP) && !(flag->bits & ON_WALL))
	{
		if (((flag->bits & WALL_ON_LEFT) && keyLeftDown) || ((flag->bits & WALL_ON_RIGHT) && keyRightDown))
		{
			flag->bits |= ON_WALL;
			SetCounter(&counters->counters, PLYR_WALL_TIME, 0);
		}
	}

	//allow a few frames to wall jump
	if (flag->bits & ON_WALL)
	{
		//first check if we can still wall jump
		uint8_t wallTime = GetCounter(counters->counters, PLYR_WALL_TIME);
		
		//don't count our wall time if we are sliding up the wall
		if (phys->vel.y < 0)
			IncrementCounter(&counters->counters, PLYR_WALL_TIME);

		if (wallTime > 15 || !((flag->bits & WALL_ON_LEFT) || (flag->bits & WALL_ON_RIGHT)))
		{
			flag->bits |= USED_WALL_JUMP;
			flag->bits &= ~ON_WALL;
			flag->bits &= ~WALL_JUMPING;
		}
		else
		{
			//if we are initiating the wall jump (pressing the opposite direction, about to jump)
			if (!(flag->bits & WALL_JUMPING) && (((flag->bits & WALL_ON_LEFT) && keyRightDown) || 
												 ((flag->bits & WALL_ON_RIGHT) && keyLeftDown)))
			{
				flag->bits |= WALL_JUMPING;
				SetCounter(&counters->counters, PLYR_WALL_TIME, 0);
			}
			if (flag->bits & WALL_JUMPING)
			{
				uint8_t wallJumpTimeLeft = IncrementCounter(&counters->counters, PLYR_WALL_TIME);
				if (wallJumpTimeLeft > 15 || ((flag->bits & WALL_ON_LEFT) && !keyRightDown) || 
											 ((flag->bits & WALL_ON_RIGHT) && !keyLeftDown))
				{
					flag->bits |= USED_WALL_JUMP;
					flag->bits &= ~ON_WALL;
					flag->bits &= ~WALL_JUMPING;
				}
			}
			if (keyAction1Pressed)
			{
				//jump
				flag->bits |= USED_WALL_JUMP;
				flag->bits &= ~ON_WALL;
				flag->bits &= ~WALL_JUMPING;

				if (flag->bits & WALL_ON_LEFT)
				{
					phys->vel.x = 0x240;
					tx->scale.x = Abs(tx->scale.x);
				}
				else
				{
					phys->vel.x = -0x240;
					tx->scale.x = -Abs(tx->scale.x);
				}

				phys->vel.y = 0x370;
				anim->name = "girl_jumping";
				anim->frameTime = 0;
				anim->loop = false;
				anim->playing = true;
			}
		}
	}


	if (flag->bits & ON_LADDER)
	{
		return;
	}

	//Check if standing on slope
	uint8_t footT = TileAtPoint({tx->pos.x, tx->pos.y});
	if (footT >= SMALL_SLOPE_LEFT1 && footT <= LARGE_SLOPE_RIGHT)
	{
		int32_t tileX = (tx->pos.x >> 12) << 12;
		int32_t tileY = ((tx->pos.y) >> 12) << 12;
		int32_t groundY = GetGroundYForTileAndPos(tileX, tileY, footT, tx->pos.x);
		if (phys->vel.y <= 0 && tx->pos.y <= groundY)
		{
			flag->bits |= ON_SLOPE;
			if (footT == LARGE_SLOPE_LEFT || footT == SMALL_SLOPE_LEFT2 || footT == SMALL_SLOPE_LEFT1)
				flag->bits |= LEFT_SLOPE;
			else
				flag->bits &= ~LEFT_SLOPE;
			phys->vel.y = 0;
			tx->pos.y = groundY;
			SetCounter(&counters->counters, PLYR_SINCE_LEFT_GROUND, 0);
			flag->bits |= ON_GROUND;
		}
	}

	//allow a small window to jump after walking off ledge (to make it less frustrating when trying to make a long jump)
	if (GetCounter(counters->counters, PLYR_SINCE_LEFT_GROUND) < 5 && phys->vel.y <= 0 && keyAction1Pressed)
	{
		if (keyAction1Pressed)
		{
			phys->vel.y = 0x380;
			anim->name = "girl_jumping";
			anim->frameTime = 0;
			anim->loop = false;
			anim->currentFrame = 0;
			flag->bits &= ~ON_GROUND;
			flag->bits |= HOLDING_JUMP;
		}
	}
	else if (flag->bits & ON_GROUND)
	{
		phys->accel.y = 0;
	}
	else
	{
		if (flag->bits & HOLDING_JUMP)
			phys->accel.y = -22;
		else
			phys->accel.y = -36;
	}

	if (flag->bits & HOLDING_JUMP)
	{
		if (phys->vel.y < 0 || !hasControl)
			flag->bits &= ~HOLDING_JUMP;
		else if (KeyReleases[KEY_Action1])
		{
			flag->bits &= ~HOLDING_JUMP;
			if (phys->vel.y > 0)
				phys->vel.y >>= 1;
		}
	}


	int canSkidVelocity = 0x200;
	if (keyDownDown && Abs(phys->vel.x) > canSkidVelocity && (flag->bits & ON_GROUND))
	{
		if (phys->vel.x < 0)
			flag->bits |= SLIDE_LEFT;
		else
			flag->bits &= ~SLIDE_LEFT;

		if (flag->bits & ON_SLOPE)
			flag->bits |= SLIDING;
		else
			flag->bits |= SKIDDING;
	}
	else if (hasControl && !KeyDown[KEY_Down])
	{
		flag->bits &= ~SKIDDING;
		flag->bits &= ~SLIDING;
	}

	int escapeFromSkidVelocity = 0x140;
	int maxVelocity = info.maxVelocity;

	int horizontalDamping = info.horizontalDamping;
	int horizontalImpulse = 36;
	if (!(flag->bits & ON_GROUND))
	{
		horizontalDamping = info.airborneHorizontalDamping;
		horizontalImpulse = 15;
	}
	if (flag->bits & WALL_JUMPING)
	{
		horizontalImpulse = 0;
	}

	phys->accel.x = 0;
	if (!(flag->bits & SLIDING) && !(flag->bits & SKIDDING))
	{
		if (keyLeftDown)
		{
			if (!(flag->bits & ON_GROUND))
			{
				//this next part is necessary because velocity max is not applied when in the air (so that sliding faster gives you a boost), but you also don't want to have an unbounded velocity in the air, since we have mid-air acceleration)
				if (phys->vel.x < -maxVelocity)
					phys->accel.x = 0;
				else
					phys->accel.x = -horizontalImpulse;
			}
			else
				phys->accel.x = -horizontalImpulse;
		}
		if (keyRightDown)
		{
			if (!(flag->bits & ON_GROUND))
			{
				//this next part is necessary because velocity max is not applied when in the air (so that sliding faster gives you a boost), but you also don't want to have an unbounded velocity in the air, since we have mid-air acceleration)
				if (phys->vel.x > maxVelocity)
					phys->accel.x = 0;
				else
					phys->accel.x = horizontalImpulse;
			}
			else
				phys->accel.x = horizontalImpulse;
		}
	}
	else if (flag->bits & SLIDING)
	{
		horizontalDamping = 4;
		if (flag->bits & LEFT_SLOPE)
		{
			if (phys->vel.x < 0)
				phys->accel.x = 25;
			else
				phys->accel.x = 10;
		}
		else
		{
			if (phys->vel.x > 0)
				phys->accel.x = -25;
			else
				phys->accel.x = -10;
		}
	}

	if ((flag->bits & SLIDING) && !(flag->bits & ON_SLOPE))
	{
		flag->bits |= SKIDDING;
		flag->bits &= ~SLIDING;
	}
	if ((flag->bits & SLIDING) && (keyAction1Down))
	{
		flag->bits &= ~SLIDING;
	}
	if (flag->bits & SKIDDING)
	{
		flag->bits &= ~SLIDING;
		horizontalDamping = 8;
		if (flag->bits & ON_SLOPE)
		{
			flag->bits &= ~SKIDDING;
			flag->bits |= SLIDING;
		}
		if (Abs(phys->vel.x) < escapeFromSkidVelocity)
		{
			flag->bits &= ~SKIDDING;
		}
	}
	if (!(flag->bits & ON_GROUND))
	{
		flag->bits &= ~SKIDDING;
		flag->bits &= ~SLIDING;
	}

	int horzImpSign = Sign(phys->accel.x);

	if (phys->vel.x > 0)
	{
		if (flag->bits & ON_GROUND)
		{
			tx->scale.x = 1;
			if ((flag->bits & SKIDDING) || (flag->bits & SLIDING))
				tx->scale.x = (flag->bits & SLIDE_LEFT) ? -1 : 1;
			if (phys->vel.x > maxVelocity)
				phys->vel.x = maxVelocity;
		}
		else
		{
			float scaleX = Sign(phys->accel.x); 
			if (scaleX != 0)
				tx->scale.x = scaleX;
		}
		if (phys->vel.x > horizontalDamping)
			phys->vel.x -= horizontalDamping;
		else
			phys->vel.x = 0;
	}
	else if (phys->vel.x < 0)
	{
		if (flag->bits & ON_GROUND)
		{
			tx->scale.x = -1;
			if ((flag->bits & SKIDDING) || (flag->bits & SLIDING))
				tx->scale.x = (flag->bits & SLIDE_LEFT) ? -1 : 1;
			if (phys->vel.x < -maxVelocity)
				phys->vel.x = -maxVelocity;
		}
		else
		{
			float scaleX = Sign(phys->accel.x); 
			if (scaleX != 0)
				tx->scale.x = scaleX;
		}
		if (phys->vel.x < -horizontalDamping)
			phys->vel.x += horizontalDamping;
		else
			phys->vel.x = 0;
	}
	else
	{
		if (horzImpSign)
			tx->scale.x = horzImpSign;
	}

	if (knockBack)
	{
		anim->name = "girl_getting_hit";
		tx->scale.x = phys->vel.x > 0 ? -1 : 1;
	}
	else if (flag->bits & ON_GROUND)
	{
		anim->playing = true;
		anim->loop = true;

		if (Abs(phys->vel.x) > 0 || horzImpSign)
		{
			if ((flag->bits & SLIDING) || (flag->bits & SKIDDING))
				anim->name = "girl_sliding";
			else
			//if (hasControl && KeyDown[KEY_Action2])
			//	anim->name = "girl_running_firing_wand";
			//else
				anim->name = "girl_running";
		}
		else
		{
			//if (hasControl && KeyDown[KEY_Action2])
			//	anim->name = "girl_firing_wand";
			//else
			{
				if (keyUpDown)
					anim->name = "girl_looking_up";
				else
					anim->name = "girl_standing";
			}
		}
	}
	else
	{
		if (!(flag->bits & HOLDING_JUMP) && phys->vel.y < -30)
		{
			anim->playing = true;
			anim->loop = true;
			//if (hasControl && KeyDown[KEY_Action2])
			//	anim->name = "girl_falling_firing_wand";
			//else
				anim->name = "girl_falling";
		}
	}

	rider->size.y = (foot3 << 9);

	if (!wasOnGround && (flag->bits & ON_GROUND) && oldYVel < -0x300)
	{
		if (phys->vel.x > 0x200)
			CreateAnimatedParticles(tx->pos, DUST_SINGLE_SIDED_PUFF, false, 0);
		else if (phys->vel.x < -0x200)
			CreateAnimatedParticles(tx->pos, DUST_SINGLE_SIDED_PUFF, true, 0);
		else
		{
			CreateAnimatedParticles(tx->pos, DUST_SINGLE_SIDED_PUFF, true, 0);
			CreateAnimatedParticles(tx->pos, DUST_SINGLE_SIDED_PUFF, false, 0);
		}
	}

	if ((flag->bits & ON_GROUND) && Abs(phys->vel.x) > 0x200)
	{
		uint8_t runningDust = IncrementCounter(&counters->counters2, PLYR_CREATE_DUST);
		if (runningDust > (rand() % 8 + 8))
		{
			SetCounter(&counters->counters2, PLYR_CREATE_DUST, 0);
			CreateAnimatedParticles(tx->pos, DUST_RUNNING_PUFF, true, 0);
		}
	}
	else
	{
		SetCounter(&counters->counters2, PLYR_CREATE_DUST, 0);
	}
}


UPDATE_FUNCTION(PlatformUpdate)
{
	auto tx = GO(transform);
	auto phys = GO(physics);
	auto counters = GO(counters);

	uint8_t mode = GetCounter(counters->counters, 0);

	switch (mode)
	{
		case 0:
		{
			phys->vel.x = 0x80;
			uint8_t time = IncrementCounter(&counters->counters, 1);
			if (time > 100)
			{
				ClearCounter(&counters->counters, 1);
				SetCounter(&counters->counters, 0, 1);
			}
		} break;

		case 1:
		{
			phys->vel.x = -0x80;
			uint8_t time = IncrementCounter(&counters->counters, 1);
			if (time > 100)
			{
				ClearCounter(&counters->counters, 1);
				SetCounter(&counters->counters, 0, 0);
			}
		} break;
	}
}

enum arrow_flags
{
	ARROW_IN_FLIGHT				= 0x1,
	ARROW_FLYING_LEFT			= 0x2,
	ARROW_FLYING_RIGHT			= 0x4,
};

enum arrow_counters
{
	ARROW_WOBBLE				= 0,
};

UPDATE_FUNCTION(ArrowUpdate)
{
	auto tx = GO(transform);
	auto phys = GO(physics);
	auto collider = GO(collider);
	auto flag = GO(custom_flags);
	auto parent = GO(parent);
	auto counters = GO(counters);


	if (flag->bits & ARROW_IN_FLIGHT)
	{
		for (int i = 0; i < ArrayCount(collider->collisions); i++)
		{
			int othID = collider->collisions[i];
			if (othID != -1)
			{
				object_type type = GameComponents.type[GameObjectIDs[othID].index];
				switch (type)
				{
					case OBJ_PLAYER:
					{
						RemoveObject(goId);
					} break;

					case OBJ_SWORD_SWIPE:
					{
						RemoveObject(goId);
					} break;

					default: {} break;
				}
			}
			else
				break;
		}


		if (flag->bits & ARROW_FLYING_LEFT)
		{
			test_point TestPoints[1] = {};
			TestPoints[0].p = {tx->pos.x, tx->pos.y + (1 << 9)};
			TestPoints[0].type = POINT_LEFT;
			PlatformTest(goId, TestPoints, 1);

			bool colLeft = false;
			int32_t colPointL = INT_MIN;

			if (TileAtPoint(TestPoints[0].p) == SQUARE)
			{
				int tileX = (tx->pos.x - (1<<9)) >> 12;
				colPointL = ((tileX + 1) << 12) + (1 << 9);
				colLeft = true;
			}
			if (TestPoints[0].hitting)
			{
				colLeft = true;
				int oldColPointL = colPointL;
				colPointL = Max(TestPoints[0].limit + (1<<9), colPointL);

				if (colPointL != oldColPointL)
				{
					parent->parentID = TestPoints[0].platformID;
					parent->offset = OTH(parent->parentID, transform)->pos - tx->pos;
				}
			}

			if (colLeft)
			{
				phys->vel.x = 0;
				collider->mask = 0;
				flag->bits &= ~ARROW_IN_FLIGHT;
				SetCounter(&counters->counters, ARROW_WOBBLE, 40);
			}
		}
		else if (flag->bits & ARROW_FLYING_RIGHT)
		{
			test_point TestPoints[1] = {};
			TestPoints[0].p = {tx->pos.x, tx->pos.y + (1 << 9)};
			TestPoints[0].type = POINT_RIGHT;
			PlatformTest(goId, TestPoints, 1);

			bool colRight = false;
			int32_t colPointR = INT_MIN;

			if (TileAtPoint(TestPoints[0].p) == SQUARE)
			{
				int tileX = (tx->pos.x + (1<<9)) >> 12;
				colPointR = ((tileX + 1) << 12) - (1 << 9);
				colRight = true;
			}
			if (TestPoints[0].hitting)
			{
				colRight = true;
				int oldColPointR = colPointR;
				colPointR = Max(TestPoints[0].limit - (1<<9), colPointR);

				if (colPointR != oldColPointR)
				{
					parent->parentID = TestPoints[0].platformID;
					parent->offset = OTH(parent->parentID, transform)->pos - tx->pos;
				}
			}

			if (colRight)
			{
				phys->vel.x = 0;
				collider->mask = 0;
				flag->bits &= ~ARROW_IN_FLIGHT;
				SetCounter(&counters->counters, ARROW_WOBBLE, 40);
			}
		}
	}
	else
	{
		if (parent->parentID > -1)
			tx->pos = OTH(parent->parentID, transform)->pos - parent->offset;
	}

	if (DecrementCounter(&counters->counters, ARROW_WOBBLE) > 0)
	{
		tx->rot = 8 * sinf(GetCounter(counters->counters, ARROW_WOBBLE) * 0.4f) * (GetCounter(counters->counters, ARROW_WOBBLE) / 20.0f);
		if (flag->bits & ARROW_FLYING_RIGHT)
			tx->rot += 180;
	}
}

void CreateArrow(ivec2 pos, bool left)
{
	int goId = AddObject(OBJ_ENEMY_PROJECTILE);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto phys = GO(physics);
	auto update = GO(update);
	auto sprite = GO(sprite);
	auto flag = GO(custom_flags);
	auto collider = GO(collider);
	auto parent = GO(parent);
	meta->cmpInUse = UPDATE | CUSTOM_FLAGS | PHYSICS | TRANSFORM | SPRITE | COLLIDER | PARENT;
	meta->flags |= GAME_OBJECT;
	InitObject(goId);

	update->update = ArrowUpdate;
	sprite->name = "Arrow";
	sprite->depth = 4;
	collider->ur = ivec2((2<<9), (1<<9));
	collider->bl = ivec2(-(1<<9), -(1<<9));
	tx->pos = pos;
	tx->rot = left ? 0 : 180;
	phys->vel.x = left ? -0x360 : 0x360;
	parent->parentID = -1;
	flag->bits |= (left ? ARROW_FLYING_LEFT : ARROW_FLYING_RIGHT);
	flag->bits |= ARROW_IN_FLIGHT;
}


UPDATE_FUNCTION(EnemySwordUpdate)
{
	auto tx = GO(transform);
	auto anim = GO(anim);
	auto parent = GO(parent);
	auto flag = GO(custom_flags);
	auto collider = GO(collider);
	tx->pos = OTH(parent->parentID, transform)->pos;

	int playerSwordID = GetFirstOfType(OBJ_SWORD_SWIPE);
	if (playerSwordID > -1)
	{
		int minX = tx->pos.x + collider->bl.x;
		int minY = tx->pos.y + collider->bl.y;
		int maxX = tx->pos.x + collider->ur.x;
		int maxY = tx->pos.y + collider->ur.y;

		auto othTx = OTH(playerSwordID, transform);
		auto othCollider = OTH(playerSwordID, collider);
		int othMinX = othTx->pos.x + othCollider->bl.x;
		int othMinY = othTx->pos.y + othCollider->bl.y;
		int othMaxX = othTx->pos.x + othCollider->ur.x;
		int othMaxY = othTx->pos.y + othCollider->ur.y;

		if (minX <= othMaxX && maxX >= othMinX && minY <= othMaxY && maxY >= othMinY)
		{
			auto othAnim = OTH(playerSwordID, anim);
			bool playerOnRight = (othTx->pos.x - tx->pos.x > 0);
			if (Abs(othAnim->frameTime - anim->frameTime) < 0.1f) //swung at the same time
			{
				CreateKnockBack(othTx->pos, (playerOnRight ? ivec2(0x400,0) : ivec2(-0x400,0)));
				CreateKnockBack(tx->pos, (playerOnRight ? ivec2(-0x400,0) : ivec2(0x400,0)));
			}
			else if (othAnim->frameTime > anim->frameTime) //player swung first
			{
				CreateKnockBack(tx->pos, (playerOnRight ? ivec2(-0x700,0) : ivec2(0x700,0)));
			}
			else //enemy swung first
			{
				CreateKnockBack(othTx->pos, (playerOnRight ? ivec2(0x700,0) : ivec2(-0x700,0)));
			}
		}
	}


	if (!(flag->bits & SWORD_INITIALIZED) && !(flag->bits & SWORD_COLLISION_TESTED))
	{
		flag->bits |= SWORD_INITIALIZED;
		collider->mask = 0xffff;
	}
	else if (!(flag->bits & SWORD_COLLISION_TESTED))
	{
		flag->bits |= SWORD_COLLISION_TESTED;
		collider->mask = 0;
	}

	if (!anim->playing)
	{
		RemoveObject(goId);
	}
}


void CreateEnemySwordSwipe(int parentID, ivec2 pos, uint32_t dir)
{
	int goId = AddObject(OBJ_ENEMY_SWORD_SWIPE);
	auto meta = GO(metadata);
	auto tx = GO(transform);
	auto anim = GO(anim);
	auto collider = GO(collider);
	auto update = GO(update);
	auto parent = GO(parent);
	auto flag = GO(custom_flags);
	meta->cmpInUse = CUSTOM_FLAGS | UPDATE | PARENT | TRANSFORM | COLLIDER | SPRITE | ANIM;
	meta->flags |= GAME_OBJECT;
	InitObject(goId);
	tx->pos = pos;
	anim->loop = false;
	anim->playing = true;
	anim->frameTime = 0;
	update->update = EnemySwordUpdate;
	flag->bits |= dir;

	if (flag->bits & SWIPE_RIGHT)
	{
		collider->ur = ivec2((20 << 9), (7 << 9));
		collider->bl = ivec2(0, -(7 << 9));
		tx->scale.x = 0.5f;
		anim->name = "sword_side";
	}
	else if (flag->bits & SWIPE_LEFT)
	{
		collider->ur = ivec2(0, (7 << 9));
		collider->bl = ivec2(-(20 << 9), -(7 << 9));
		tx->scale.x = -0.5f;
		anim->name = "sword_side";
	} 
	else if (flag->bits & SWIPE_UP)
	{
		collider->ur = ivec2((7 << 9), (20 << 9));
		collider->bl = ivec2(-(7 << 9), 0);
		tx->scale.y = 0.5f;
		anim->name = "sword_up";
	} 
	else if (flag->bits & SWIPE_DOWN)
	{
		collider->ur = ivec2((7 << 9), 0);
		collider->bl = ivec2(-(7 << 9), -(20 << 9));
		tx->scale.y = 0.5f;
		anim->name = "sword_down";
	} 

	collider->mask = 0;
	parent->parentID = parentID;
}

enum standin_flags
{
	STANDIN_DEAD					= 0x100000,
};

enum standin_counters
{
};


UPDATE_FUNCTION(StandinUpdate)
{
	auto phys = GO(physics);
	auto tx = GO(transform);
	auto flag = GO(custom_flags);
	auto sprite = GO(sprite);
	auto counters = GO(counters);
	auto collider = GO(collider);
	auto rider = GO(rides_platforms);

	if (strcmp(sprite->name, "EnemyStandinDamaged") == 0)
	{
		uint8_t deadTime = DecrementCounter(&counters->counters, 0);
		if (deadTime % 10 == 1)
		{
			CreateAnimatedParticles(tx->pos, BLOOD_SPLATTER_UP, true, phys->vel.x > 0 ? 70 : -70);
		}
		if (phys->vel.y == 0 && deadTime == 0)
		{
			sprite->name = "EnemyStandinCorpse";
		}
	}


	for (int i = 0; i < ArrayCount(collider->collisions); i++)
	{
		int othID = collider->collisions[i];
		if (othID != -1)
		{
			object_type type = GameComponents.type[GameObjectIDs[othID].index];
			switch (type)
			{
				case OBJ_KNOCKBACK:
				{
					phys->vel = OTH(othID, physics)->vel;
				} break;

				case OBJ_SWORD_SWIPE:
				{
					collider->mask = 0;
					sprite->name = "EnemyStandinDamaged";
					SetCounter(&counters->counters, 0, 40);
					auto othFlag = OTH(othID, custom_flags);

					CreateGameFreezer(2, 8);
					flag->bits |= STANDIN_DEAD;

					if (othFlag->bits & SWIPE_LEFT)
					{
						phys->vel.x = -0x350;
						phys->vel.y = 0x210;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_SIDE, false, 0);
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_UP, true, 70);
						CreateAnimatedParticles(tx->pos + ivec2((2 << 9), (3 << 9)), BLOOD_SPLATTER_UP, true, 70);
						CreateAnimatedParticles(tx->pos + ivec2(-(3 << 9), (6 << 9)), BLOOD_SPLATTER_UP, true, 70);
					}
					else if (othFlag->bits & SWIPE_RIGHT)
					{
						phys->vel.x = 0x350;
						phys->vel.y = 0x210;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
						CreateAnimatedParticles(tx->pos + ivec2(-(6 << 9), (8 << 9)), BLOOD_SPLATTER_SIDE, true, 0);
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_UP, true, -70);
						CreateAnimatedParticles(tx->pos + ivec2((2 << 9), (3 << 9)), BLOOD_SPLATTER_UP, true, -70);
						CreateAnimatedParticles(tx->pos + ivec2(-(3 << 9), (6 << 9)), BLOOD_SPLATTER_UP, true, -70);
					}
					else if (othFlag->bits & SWIPE_UP)
					{
						phys->vel.x = (rand() % 250) - 125;
						phys->vel.y = 0x320;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
					}
					else if (othFlag->bits & SWIPE_DOWN)
					{
						phys->vel.x = (rand() % 250) - 125;
						phys->vel.y = -0x200;
						flag->bits &= ~ON_SLOPE;
					}
				} break;

				default: {} break;
			}
		}
		else
			break;
	}

	tile_collision_info tileInfo;
	tileInfo.maxFallSpeed = -0x600;
	tileInfo.headHalfWidth = 3072;
	tileInfo.headHeight = 6144;
	tileInfo.bodyHalfWidth = 4096;
	tileInfo.gravity = -36;
	tileInfo.maxVelocity = 0x400;
	tileInfo.horizontalDamping = 20;
	tileInfo.airborneHorizontalDamping = 6;

	EnemyTileCollisions(goId, &tileInfo);
}

enum jumper_enemy_counters
{
	JUMPER_TIME_TIL_JUMP		= 0,
	JUMPER_FIRE_ARROW			= 1,
};

enum jumper_enemy_flags
{
	JUMPER_DEAD					= 0x100000,
	JUMPER_JUMPING				= 0x200000,
};



UPDATE_FUNCTION(JumperUpdate)
{
	auto phys = GO(physics);
	auto tx = GO(transform);
	auto flag = GO(custom_flags);
	auto sprite = GO(sprite);
	auto counters = GO(counters);
	auto collider = GO(collider);
	auto rider = GO(rides_platforms);

	auto playerID = GetFirstOfType(OBJ_PLAYER);

	if (!(flag->bits & JUMPER_DEAD) && (flag->bits & JUMPER_JUMPING) && (flag->bits & ON_GROUND))
	{
		sprite->name = "JumpingEnemy1";
		flag->bits &= ~JUMPER_JUMPING;
	}

	if (!(flag->bits & JUMPER_DEAD) && (flag->bits & ON_GROUND))
	{
		if (IncrementCounter(&counters->counters2, JUMPER_FIRE_ARROW) == 250)
		{
			SetCounter(&counters->counters2, JUMPER_FIRE_ARROW, 0);
			if (playerID > -1)
			{
				bool playerOnRight = OTH(playerID, transform)->pos.x - tx->pos.x > 0;
				CreateArrow(tx->pos + (playerOnRight ? ivec2(9<<9, 8<<9) : ivec2(-(9<<9),8<<9)), (playerOnRight ? false : true));
			}
		}
		if (DecrementCounter(&counters->counters2, JUMPER_TIME_TIL_JUMP) == 0)
		{
			if (playerID > -1)
			{
				auto playerTx = OTH(playerID, transform);
				if (Abs(playerTx->pos.x - tx->pos.x) < (80 << 9))
				{
					bool jumpRight = playerTx->pos.x - tx->pos.x > 0;
					phys->vel.x = jumpRight ? 0x320 : -0x320;
					phys->vel.y = 0x380;
					tx->pos.y += (2 << 9);
					tx->scale.x = jumpRight ? -1 : 1;
					flag->bits &= ~ON_SLOPE;
					flag->bits &= ~ON_GROUND;
					SetCounter(&counters->counters2, JUMPER_TIME_TIL_JUMP, 120);
					flag->bits |= JUMPER_JUMPING;
					sprite->name = "JumpingEnemy2";
				}
			}
		}
	}

	if (strcmp(sprite->name, "JumpingEnemy3") == 0)
	{
		uint8_t deadTime = DecrementCounter(&counters->counters, 0);
		if (deadTime % 10 == 1)
		{
			CreateAnimatedParticles(tx->pos, BLOOD_SPLATTER_UP, true, phys->vel.x > 0 ? 70 : -70);
		}
		if (phys->vel.y == 0 && deadTime == 0)
		{
			sprite->name = "JumpingEnemy4";
		}
	}


	for (int i = 0; i < ArrayCount(collider->collisions); i++)
	{
		int othID = collider->collisions[i];
		if (othID != -1)
		{
			object_type type = GameComponents.type[GameObjectIDs[othID].index];
			switch (type)
			{
				case OBJ_SWORD_SWIPE:
				{
					collider->mask = 0;
					sprite->name = "JumpingEnemy3";
					SetCounter(&counters->counters, 0, 40);
					auto othFlag = OTH(othID, custom_flags);

					flag->bits |= JUMPER_DEAD;
					CreateGameFreezer(2, 8);

					if (othFlag->bits & SWIPE_LEFT)
					{
						phys->vel.x = -0x350;
						phys->vel.y = 0x210;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_SIDE, false, 0);
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_UP, true, 70);
						CreateAnimatedParticles(tx->pos + ivec2((2 << 9), (3 << 9)), BLOOD_SPLATTER_UP, true, 70);
						CreateAnimatedParticles(tx->pos + ivec2(-(3 << 9), (6 << 9)), BLOOD_SPLATTER_UP, true, 70);
					}
					else if (othFlag->bits & SWIPE_RIGHT)
					{
						phys->vel.x = 0x350;
						phys->vel.y = 0x210;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
						CreateAnimatedParticles(tx->pos + ivec2(-(6 << 9), (8 << 9)), BLOOD_SPLATTER_SIDE, true, 0);
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_UP, true, -70);
						CreateAnimatedParticles(tx->pos + ivec2((2 << 9), (3 << 9)), BLOOD_SPLATTER_UP, true, -70);
						CreateAnimatedParticles(tx->pos + ivec2(-(3 << 9), (6 << 9)), BLOOD_SPLATTER_UP, true, -70);
					}
					else if (othFlag->bits & SWIPE_UP)
					{
						phys->vel.x = (rand() % 250) - 125;
						phys->vel.y = 0x320;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
					}
					else if (othFlag->bits & SWIPE_DOWN)
					{
						phys->vel.x = (rand() % 250) - 125;
						phys->vel.y = -0x200;
						flag->bits &= ~ON_SLOPE;
					}
				} break;

				default: {} break;
			}
		}
		else
			break;
	}


	tile_collision_info tileInfo;
	tileInfo.maxFallSpeed = -0x600;
	tileInfo.headHalfWidth = 3072;
	tileInfo.headHeight = 6144;
	tileInfo.bodyHalfWidth = 4096;
	tileInfo.gravity = -36;
	tileInfo.maxVelocity = 0x400;
	tileInfo.horizontalDamping = 20;
	tileInfo.airborneHorizontalDamping = 6;

	EnemyTileCollisions(goId, &tileInfo);
}

enum swooper_flags
{
	SWOOPER_DEAD				= 0x100000,
	SWOOPER_SWOOPING			= 0x200000,
};

enum swooper_counters
{
	SWOOPER_BEHAVIOR_MODE			= 0,
	SWOOPER_BEHAVIOR_MODE_COUNT		= 1,
	SWOOPER_RANDOM_DIR_COUNT		= 2,
};

UPDATE_FUNCTION(SwooperSaveAndLoad)
{
	if (SaveOrLoad == LOAD)
	{
		if (LoadData)
		{
			//format [numWaypoints, [waypoint0.x, waypoint0.y], ...]
			auto waypoints = GO(waypoints);
			json_array *array = LoadData->array;
			waypoints->count = array->GetByIndex(0)->number.i;
			for (int i = 0; i < waypoints->count; i++)
			{
				json_array *point = array->GetByIndex(i+1)->array;
				waypoints->points[i] = ivec2(GetJSONValAsInt(point->GetByIndex(0)), GetJSONValAsInt(point->GetByIndex(1)));
			}
		}
	}
	else if (SaveOrLoad == SAVE)
	{
		auto waypoints = GO(waypoints);
		sprintf(SaveData, "[ %d, [%d, %d], [%d, %d], [%d, %d] ]", waypoints->count, waypoints->points[0].x, waypoints->points[0].y, 
																					waypoints->points[1].x, waypoints->points[1].y, 
																					waypoints->points[2].x, waypoints->points[2].y);
	}
}

UPDATE_FUNCTION(SwooperUpdate)
{
	auto phys = GO(physics);
	auto tx = GO(transform);
	auto flag = GO(custom_flags);
	auto sprite = GO(sprite);
	auto counters = GO(counters);
	auto collider = GO(collider);
	auto rider = GO(rides_platforms);
	auto waypoints = GO(waypoints);

	auto playerID = GetFirstOfType(OBJ_PLAYER);

	if (!(flag->bits & SWOOPER_DEAD))
	{
		if (IncrementCounter(&counters->counters2, SWOOPER_RANDOM_DIR_COUNT) > 60)
		{
			float randomAngle = (rand() % 360) * DEGREES_TO_RADIANS;
			ivec2 random = ivec2(0x60 * sinf(randomAngle), 0x60 * cosf(randomAngle));
			phys->vel += random;
			SetCounter(&counters->counters2, SWOOPER_RANDOM_DIR_COUNT, 0);
		}

		ivec2 dir;
		uint8_t mode = GetCounter(counters->counters2, SWOOPER_BEHAVIOR_MODE);
		switch (mode)
		{
			case 0: //moving toward waypoint 0
			case 1: //moving toward waypoint 1
			{
				dir = waypoints->points[mode] - tx->pos;
				if (length(vec2(dir)) < 0x300)
				{
					SetCounter(&counters->counters2, SWOOPER_BEHAVIOR_MODE, (mode + 1) % 2);
				}
				//determine if we want to chase the player
			} break;

			case 2: //chasing player
			{
				if (playerID > -1)
					dir = OTH(playerID, transform)->pos + ivec2(0, 32<<9) - tx->pos; //move to a point 32 pixels above player
				else
					SetCounter(&counters->counters2, SWOOPER_BEHAVIOR_MODE, 3);

			} break;

			case 3: //swooping
			{
			} break;

			case 4: //recovery
			{
			} break;

		}

		if (mode == 0 || mode == 1 || mode == 2)
		{
			vec2 dirf = vec2(dir);
			if (length(dirf) != 0)
			{
				dirf = normalize(dirf);
				vec2 accel = 18.0f * dirf;
				phys->accel = ivec2(accel);
				vec2 vel = ivec2(phys->vel.x, phys->vel.y);
				if (length(vel) > 0x160)
					vel = (float)0x160 * normalize(vel);
				phys->vel = ivec2(vel);
			}
		}
	}

	if (strcmp(sprite->name, "Swooper2") == 0)
	{
		uint8_t deadTime = DecrementCounter(&counters->counters, 0);
		if (deadTime % 10 == 1)
		{
			CreateAnimatedParticles(tx->pos, BLOOD_SPLATTER_UP, true, phys->vel.x > 0 ? 70 : -70);
		}
		if (phys->vel.y == 0 && deadTime == 0)
		{
			sprite->name = "Swooper3";
		}
	}


	for (int i = 0; i < ArrayCount(collider->collisions); i++)
	{
		int othID = collider->collisions[i];
		if (othID != -1)
		{
			object_type type = GameComponents.type[GameObjectIDs[othID].index];
			switch (type)
			{
				case OBJ_SWORD_SWIPE:
				{
					collider->mask = 0;
					sprite->name = "Swooper2";
					SetCounter(&counters->counters, 0, 40);
					auto othFlag = OTH(othID, custom_flags);

					flag->bits |= SWOOPER_DEAD;
					phys->accel = ivec2(0, 0);
					CreateGameFreezer(2, 8);

					if (othFlag->bits & SWIPE_LEFT)
					{
						phys->vel.x = -0x350;
						phys->vel.y = 0x210;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_SIDE, false, 0);
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_UP, true, 70);
						CreateAnimatedParticles(tx->pos + ivec2((2 << 9), (3 << 9)), BLOOD_SPLATTER_UP, true, 70);
						CreateAnimatedParticles(tx->pos + ivec2(-(3 << 9), (6 << 9)), BLOOD_SPLATTER_UP, true, 70);
					}
					else if (othFlag->bits & SWIPE_RIGHT)
					{
						phys->vel.x = 0x350;
						phys->vel.y = 0x210;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
						CreateAnimatedParticles(tx->pos + ivec2(-(6 << 9), (8 << 9)), BLOOD_SPLATTER_SIDE, true, 0);
						CreateAnimatedParticles(tx->pos + ivec2((6 << 9), (8 << 9)), BLOOD_SPLATTER_UP, true, -70);
						CreateAnimatedParticles(tx->pos + ivec2((2 << 9), (3 << 9)), BLOOD_SPLATTER_UP, true, -70);
						CreateAnimatedParticles(tx->pos + ivec2(-(3 << 9), (6 << 9)), BLOOD_SPLATTER_UP, true, -70);
					}
					else if (othFlag->bits & SWIPE_UP)
					{
						phys->vel.x = (rand() % 250) - 125;
						phys->vel.y = 0x320;
						tx->pos.y += (2 << 9);
						flag->bits &= ~ON_SLOPE;
						flag->bits &= ~ON_GROUND;
					}
					else if (othFlag->bits & SWIPE_DOWN)
					{
						phys->vel.x = (rand() % 250) - 125;
						phys->vel.y = -0x200;
						flag->bits &= ~ON_SLOPE;
					}
				} break;

				default: {} break;
			}
		}
		else
			break;
	}


	tile_collision_info tileInfo;
	tileInfo.maxFallSpeed = -0x600;
	tileInfo.headHalfWidth = 2048;
	tileInfo.headHeight = 4608;
	tileInfo.bodyHalfWidth = 3072;
	tileInfo.maxVelocity = 0x400;
	if (flag->bits & SWOOPER_DEAD)
	{
		tileInfo.gravity = -36;
		tileInfo.horizontalDamping = 20;
		tileInfo.airborneHorizontalDamping = 2;
		tileInfo.setXScale = true;
	}
	else
	{
		tileInfo.gravity = phys->accel.y;
		tileInfo.horizontalDamping = 3;
		tileInfo.airborneHorizontalDamping = 3;
		tileInfo.setXScale = false;
	}

	EnemyTileCollisions(goId, &tileInfo);
}

UPDATE_FUNCTION(DoorSaveAndLoad)
{
	if (SaveOrLoad == LOAD)
	{
		if (LoadData)
		{
			//format [numWaypoints, [waypoint0.x, waypoint0.y], ...]
			auto str = GO(string_storage);
			json_array *array = LoadData->array;
			str->string = array->GetByIndex(0)->string;
		}
	}
	else if (SaveOrLoad == SAVE)
	{
		auto str = GO(string_storage);
		sprintf(SaveData, "[ \"%s\" ]", str->string);
	}
}


UPDATE_FUNCTION(CameraUpdate)
{
	local_persistent bool init = false;
	if (!init)
	{
		init = true;
		GameCamera = {(int)Camera.pos.x, (int)Camera.pos.y};
		GameCamFocus = {(int)Camera.pos.x, (int)Camera.pos.y};
		FocusFunction = PlayerFocus;
		CameraUpdateFunction = CameraLerpFunction;
	}

	GameCamFocus = FocusFunction();

	CameraUpdateFunction();
}


struct rain_drop
{
	ivec2 pos;
	ivec2 vel;
	ivec2 accel;
};

global_variable rain_drop RainDrops[1000];
global_variable int NumRainDrops;
global_variable ivec2 RainSplashes[1000];
global_variable int NumRainSplashes;

UPDATE_FUNCTION(RainUpdateAndDraw)
{

	int NumRainDropsToMake = rand() % 3 + 1;
	for (int i = 0; i < NumRainDropsToMake; i++)
	{
		if (NumRainDrops < ArrayCount(RainDrops))
		{
			rain_drop *drop = &RainDrops[NumRainDrops++];
			drop->pos = {Camera.pos.x + (((rand() % 600) - 300) << 9), Camera.pos.y + (200 << 9)};
			drop->vel = {0, -0x500};
		}
	}
	uint32_t dropColor = MAKE_COLOR(150,200,255,200);

	SetShader("basic");
	for (int i = 0; i < NumRainSplashes; i++)
	{
		ivec2 *splash = &RainSplashes[i];
		//Draw the splash
		{
			float x0 = splash->x - 2;
			float x1 = x0+1;
			float y0 = splash->y + 1;
			float y1 = y0+1;
			DrawQuad(dropColor, {x0,y0}, {x1,y0}, {x1,y1}, {x0,y1});
		}
		{
			float x0 = splash->x + 2;
			float x1 = x0+1;
			float y0 = splash->y + 1;
			float y1 = y0+1;
			DrawQuad(dropColor, {x0,y0}, {x1,y0}, {x1,y1}, {x0,y1});
		}
	}
	NumRainSplashes = 0;

	for (int i = 0; i < NumRainDrops; i++)
	{
		rain_drop *drop = &RainDrops[i];
		drop->pos.x += drop->vel.x;
		drop->pos.y += drop->vel.y;

		uint32_t newY;
		if (CollidingWithTileAtPoint(drop->pos.x, drop->pos.y, &newY))
		{
			ivec2 *splash = &RainSplashes[NumRainSplashes++];
			*splash = {drop->pos.x, newY};
			//Draw the splash
			{
				int x0 = splash->x - 0x200;
				int x1 = x0+0x200;
				int y0 = splash->y + 0x200;
				int y1 = y0+0x200;
				DrawQuad(dropColor, ToScreen({x0,y0}), ToScreen({x1,y0}), ToScreen({x1,y1}), ToScreen({x0,y1}));
			}
			{
				int x0 = splash->x + 0x200;
				int x1 = x0+0x200;
				int y0 = splash->y + 0x200;
				int y1 = y0+0x200;
				DrawQuad(dropColor, ToScreen({x0,y0}), ToScreen({x1,y0}), ToScreen({x1,y1}), ToScreen({x0,y1}));
			}


			RainDrops[i] = RainDrops[NumRainDrops-1];
			NumRainDrops--;
			i--;
		}
		else if (drop->pos.y < Camera.pos.y - (200 << 9))
		{
			RainDrops[i] = RainDrops[NumRainDrops-1];
			NumRainDrops--;
			i--;
		}
		else
		{
			int x0 = drop->pos.x;
			int x1 = x0+0x200;
			int y0 = drop->pos.y;
			int y1 = y0+0x600;
			DrawQuad(dropColor, ToScreen({x0,y0}), ToScreen({x1,y0}), ToScreen({x1,y1}), ToScreen({x0,y1}));
		}
	}
}


UPDATE_FUNCTION(DisplayDebugMessage)
{
	if (DebugMessageCounter > 0)
	{
		DebugMessageCounter--;
		SetShader("basic_tex_color");
		SetFontAsPercentageOfScreen("JackInput", 2.4f);
		SetFontColor(MAKE_COLOR(255,100,0,255));
		DrawTextGui(2, 97, DebugMessage);
	}
}

enum tile_drawer_flags
{
	TILE_DRAWER_DRAW_FRONT  = 0x1,
	TILE_DRAWER_DRAW_BACK	= 0x2,
};

UPDATE_FUNCTION(TileDrawer)
{
	auto flag = GO(custom_flags);
	//Draw tiles
	{
		//get corners of view
		int tileStartX = (int)(Camera.pos.x - Camera.width) >> 12;
		int tileEndX = (int)(Camera.pos.x + Camera.width) >> 12;
		int tileStartY = (int)(Camera.pos.y - Camera.height) >> 12;
		int tileEndY = (int)(Camera.pos.y + Camera.height) >> 12;

		tileStartX = Max(Min(MAP_W - 1, tileStartX), 0);
		tileEndX   = Max(Min(MAP_W - 1, tileEndX), 0);
		tileStartY = Max(Min(MAP_H - 1, tileStartY), 0);
		tileEndY   = Max(Min(MAP_H - 1, tileEndY), 0);

		SetShader("basic_tex_color");
		int curTileSet = -1;
		tile_paint *curTilePaint;
		game_tile_sprites *s;
		sprite_atlas *atlas;
		float u0, v0, u1, v1;
		bool frontDrawer = flag->bits & TILE_DRAWER_DRAW_FRONT;
		bool backDrawer = flag->bits & TILE_DRAWER_DRAW_BACK;

		for (int y = tileStartY; y < tileEndY; y++)
		{
			for (int x = tileStartX; x < tileEndX; x++)
			{
				uint32_t t = TileMapV[y*MAP_W + x];
				uint8_t flags = TileFlags(t);
				if (!(flags & TILE_IN_USE))
					continue;

				uint8_t set = TileSet(t);
				if (set != curTileSet)
				{
					curTileSet = set;
					//set texture
					curTilePaint = &TilePaint[set];
					s = (game_tile_sprites *)GetFromHash(&TileSprites, curTilePaint->name);
					atlas = &SpritePack.atlas[s->atlas_index];
					SetTexture(atlas->tex);
				}

				if ((flags & HAS_FRONT) && frontDrawer)
				{
					game_tile *tile = &s->tiles[TileNumF(t)];
					DrawTile(x, y, tile->u0, tile->u1, tile->v0, tile->v1, Camera.pos);
				}
				if ((flags & HAS_BACK) && backDrawer)
				{
					game_tile *tile = &s->tiles[TileNumB(t)];
					DrawTile(x, y, tile->u0, tile->u1, tile->v0, tile->v1, Camera.pos);
				}
			}
		}
	}
}

enum editor_mode
{
	NONE,
	TILE_PLACE,
	CAMERA_RESTRAINT,
	DISPLAY_PREFAB_MENU,
	MOVE_PREFABS,
	RUNNING_GAME,
	SELECTING_LEVEL,
	EDIT_OBJECTS,
	DEBUG_FUNCTIONS,
};

const char *editor_mode_names[] = {
	"NONE",
	"TILE_PLACE",
	"CAMERA_RESTRAINT",
	"DISPLAY_PREFAB_MENU",
	"MOVE_PREFABS",
	"RUNNING_GAME",
	"SELECTING_LEVEL",
	"EDIT_OBJECTS",
	"DEBUG_FUNCTIONS",
};

global_variable editor_mode EditorMode;
global_variable int TilePlaceID;
global_variable int CameraRestraintID;
global_variable int PrefabSelectID;
global_variable int MovePrefabID;
global_variable int LevelSelectID;
global_variable int EditObjectID;
global_variable int DebugFunctionsID;


global_variable bool SavedLevel;
global_variable char LevelName[64];

UPDATE_FUNCTION(LevelSaver)
{
	if (KeyboardPresses[KB_S] && CtrlHeld && !(EditorMode == RUNNING_GAME))
	{
		sprintf(DebugMessage, "Saving...");
		DebugMessageCounter = 60;
		SerializeLevel(&SavedLevel, LevelName);
	}
}

void ExitEditorMode()
{
	switch (EditorMode)
	{
		case NONE: {} break;
		case TILE_PLACE:
		{
			RemoveObject(TilePlaceID);
		} break;
		case CAMERA_RESTRAINT:
		{
			RemoveObject(CameraRestraintID);
			Camera.width *= 0.5f;
			Camera.height *= 0.5f;
		} break;
		case DISPLAY_PREFAB_MENU:
		{
			RemoveObject(PrefabSelectID);
		} break;
		case MOVE_PREFABS:
		{
			RemoveObject(MovePrefabID);
		} break;
		case RUNNING_GAME:
		{
			SendingGameUpdateEvents = false;
			LevelLoaderInitialized = false;
			OnLevel = LoadedLevel;
		} break;
		case SELECTING_LEVEL:
		{
			RemoveObject(LevelSelectID);
		} break;
		case EDIT_OBJECTS:
		{
			RemoveObject(EditObjectID);
		} break;
		case DEBUG_FUNCTIONS:
		{
			RemoveObject(DebugFunctionsID);
		} break;
	}
	EditorMode = NONE;
	FocusFunction = FreelyMoveableFocus;
	CameraUpdateFunction = CameraLerpFunction;
}


UPDATE_FUNCTION(DebugFunctionsUpdateAndDraw)
{
	local_persistent int DebugFunctionSelect = 0;
	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));

	int numDebugFunctions = 0;
	float curY = 94;

	{
		if (DebugFunctionSelect == numDebugFunctions++)
		{
			DrawTextGui(2, curY, " >>");
			if (KeyboardPresses[KB_ENTER])
			{
			}
		}
		DrawTextGui(5, curY, "Reload Data Pak");
	}
	
		
	if (KeyboardPresses[KB_UP])
		DebugFunctionSelect = Max(0, DebugFunctionSelect - 1);
	if (KeyboardPresses[KB_DOWN])
		DebugFunctionSelect = Min(numDebugFunctions - 1, DebugFunctionSelect + 1);
}

UPDATE_FUNCTION(LevelSelect)
{
	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));

	size_t iter;
	void *value;
	iter = HashGetFirst(&Levels, NULL, NULL, &value);

	float curY = 94;
	int curLevel = 0;
	while (value)
	{
		json_data_file *file = (json_data_file *)value;
		if (OnLevel == curLevel)
			DrawTextGui(2, curY, " >> %s", file->fileName);
		else
			DrawTextGui(2, curY, "    %s", file->fileName);

		if (KeyboardReleases[KB_ENTER] && OnLevel == curLevel)
		{
			LevelLoaderInitialized = false;
			ExitEditorMode();
			break;
		}

		curY -= 3;
		curLevel++;
		HashGetNext(&Levels, NULL, NULL, &value, iter);
	}

	//curLevel == total number of levels
	if (KeyboardPresses[KB_UP])
		OnLevel = Max(0, OnLevel - 1);
	if (KeyboardPresses[KB_DOWN])
		OnLevel = Min(curLevel - 1, OnLevel + 1);

	HashEndIteration(iter); 
}

UPDATE_FUNCTION(LevelLoader)
{
	if (!LevelLoaderInitialized)
	{
		size_t iter;
		void *value;
		char *levelName = LevelName;

		int curLevel = 0;

		iter = HashGetFirst(&Levels, NULL, NULL, &value);
		while (value)
		{
			if (curLevel == OnLevel)
			{
				json_data_file *file = (json_data_file *)value;
				SavedLevel = true;
				strcpy(LevelName, file->fileName);

				{
					//remove any old objects
					for (int i = 0; i < idCount; i++)
					{
						if (GameObjectIDs[i].inUse && (OTH(i,metadata)->flags & GAME_OBJECT))
						{
							RemoveObject(i);
						}
					}

					ProcessObjectRemovals();
					
					NumLevelObjects = 0;
					for (int x = 0; x < MAP_W; x++)
					{
						for (int y = 0; y < MAP_H; y++)
						{
							TileMapV[y*MAP_W + x] = 0;
							TileMapI[y*MAP_W + x] = 0;
						}
					}
					NumCameraRestraintAreas = 0;

					ReadInJsonDataFromDirectory(&LevelFolder, &Levels);

					json_value *initialCamera = file->val->hash->GetByKey("InitialCamera");
					InitialCameraPos = ivec2(4000<<9,4000<<9);
					if (initialCamera)
						InitialCameraPos = ivec2(GetJSONValAsInt(initialCamera->array->GetByIndex(0)), GetJSONValAsInt(initialCamera->array->GetByIndex(1)));

					Camera.pos = InitialCameraPos;
					GameCamFocus = Camera.pos;
					GameCamera = Camera.pos;

					json_value *cameraAreas = file->val->hash->GetByKey("CameraRestraints");
					if (cameraAreas)
					{
						json_value *item = cameraAreas->array->first;
						while (item)
						{
							irect *area = &CameraRestraintAreas[NumCameraRestraintAreas++];
							json_value *UR = item->array->GetByIndex(0);
							area->UR.x = UR->array->GetByIndex(0)->number.i;
							area->UR.y = UR->array->GetByIndex(1)->number.i;
							json_value *BL = item->array->GetByIndex(1);
							area->BL.x = BL->array->GetByIndex(0)->number.i;
							area->BL.y = BL->array->GetByIndex(1)->number.i;
							item = item->next;
						}
					}

					//load objects
					json_value *objects = file->val->hash->GetByKey("Objects");
					json_value *item = objects->array->first;
					while (item)
					{
						level_objects *obj = &LevelObjects[NumLevelObjects++];
						obj->prefab = item->array->GetByIndex(0)->string;
						obj->pos = {GetJSONValAsInt(item->array->GetByIndex(1)), GetJSONValAsInt(item->array->GetByIndex(2))};

						json_data_file *prefab = (json_data_file *)GetFromHash(&PrefabData, obj->prefab);
						assert(prefab);
						obj->objectID = DeserializeObject(prefab->val);
						OTH(obj->objectID, transform)->pos = obj->pos;
						OTH(obj->objectID, metadata)->flags |= GAME_OBJECT;

						auto metadata = OTH(obj->objectID, metadata);
						if (metadata->cmpInUse & SAVE_AND_LOAD)
						{
							json_value *serializedData = item->array->num_elements == 4 ? item->array->GetByIndex(3) : NULL;
							auto saveandload = OTH(obj->objectID, save_and_load);
							SaveOrLoad = LOAD;
							LoadData = serializedData;
							saveandload->in_out(obj->objectID);
						}

						item = item->next;
					}
				}


				{
					//load tiles
					int startX, startY, pitch;
					startX = GetJSONValAsInt(file->val->hash->GetByKey("TileX"));
					startY = GetJSONValAsInt(file->val->hash->GetByKey("TileY"));
					pitch = GetJSONValAsInt(file->val->hash->GetByKey("Pitch"));

					json_value *tiles = file->val->hash->GetByKey("Tiles");
					json_value *item = tiles->array->first;
					int x = startX;
					int y = startY;
					int counter = 0;
					while (item)
					{
						TileMapV[y*MAP_W + x] = GetJSONValAsUint32(item);
						item = item->next;
						assert(item); //we will guarantee the interactive val is stashed too
						TileMapI[y*MAP_W + x] = (uint8_t)GetJSONValAsUint32(item);
						x++;
						if (++counter == pitch)
						{
							y++;
							x = startX;
							counter = 0;
						}
						item = item->next;
					}
				}
				LevelLoaderInitialized = true;
				LoadedLevel = curLevel;
				break;
			}
			curLevel++;
			HashGetNext(&Levels, NULL, NULL, &value, iter);
		}
		HashEndIteration(iter); 
	}
}





global_variable uint8_t CurrentSlopeType;
global_variable tile_sets UsingTileSet;
global_variable int UsingPaintBrush;

UPDATE_FUNCTION(TilePlacerEditorDraw)
{
	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));
	DrawTextGui(2, 13, "Controls");
	DrawTextGui(2, 10, ", . to change tile brush");
	DrawTextGui(2, 7, "hold O - set as one-way tile");
	DrawTextGui(2, 4, "hold T - set as ladder top");
	DrawTextGui(2, 1, "hold E - set as empty");
}

UPDATE_FUNCTION(TilePlacerDraw)
{

	//Draw the pallette
	//
	//Get size of pallette
	ivec2 palletteSize = {};
	for (int i = 0; i < NumTilePaintBrushes[UsingTileSet]; i++)
	{
		tile_paint_brush *paintBrush = &TilePaintBrush[UsingTileSet][i];
		tile_brush *brush = &Brushes[UsingTileSet][paintBrush->fromBrushes[0]];
		int brushMinX = INT_MAX;
		int brushMaxX = INT_MIN;
		int brushMinY = INT_MAX;
		int brushMaxY = INT_MIN;
		for (int j = 0; j < brush->numParts; j++)
		{
			if (brush->parts[j].xOffset < brushMinX) brushMinX = brush->parts[j].xOffset;
			if (brush->parts[j].xOffset > brushMaxX) brushMaxX = brush->parts[j].xOffset;
			if (brush->parts[j].yOffset < brushMinY) brushMinY = brush->parts[j].yOffset;
			if (brush->parts[j].yOffset > brushMaxY) brushMaxY = brush->parts[j].yOffset;
		}
		int brushW = brushMaxX - brushMinX + 1;
		int brushH = brushMaxY - brushMinY + 1;
		palletteSize.x += (1 + (brushW << 3)) << 9;
		if (palletteSize.y < (brushH << 12)) palletteSize.y = (brushH << 12);
	}
	palletteSize.y += 2 << 9;
	palletteSize.x += 1 << 9;

	SetShader("basic");
	ivec2 palletteBL = Camera.pos + ivec2(-Camera.width>>1,(Camera.height>>1) - palletteSize.y);
	ivec2 pcornerBL = palletteBL;
	ivec2 pcornerBR = {palletteBL.x + palletteSize.x, palletteBL.y};
	ivec2 pcornerUR = palletteBL + palletteSize;
	ivec2 pcornerUL = {palletteBL.x, palletteBL.y + palletteSize.y};
	DrawQuad(COL_WHITE, ToScreen(pcornerBL), ToScreen(pcornerBR), ToScreen(pcornerUR), ToScreen(pcornerUL));
	DrawQuad(COL_BLACK, ToScreen(pcornerBL + ivec2(0x100,0x100)), ToScreen(pcornerBR + ivec2(-0x100,0x100)), 
						ToScreen(pcornerUR + ivec2(-0x100,-0x100)), ToScreen(pcornerUL + ivec2(0x100,-0x100)));

	SetShader("basic_tex_color");

	//set texture
	tile_paint *curTilePaint = &TilePaint[UsingTileSet];
	game_tile_sprites *s = (game_tile_sprites *)GetFromHash(&TileSprites, curTilePaint->name);
	sprite_atlas *atlas = &SpritePack.atlas[s->atlas_index];
	SetTexture(atlas->tex);


	int curPosX = palletteBL.x + (1<<9);
	for (int i = 0; i < NumTilePaintBrushes[UsingTileSet]; i++)
	{
		tile_paint_brush *paintBrush = &TilePaintBrush[UsingTileSet][i];
		tile_brush *brush = &Brushes[UsingTileSet][paintBrush->fromBrushes[0]];

		int brushMinX = INT_MAX;
		int brushMaxX = INT_MIN;
		int brushMinY = INT_MAX;
		for (int j = 0; j < brush->numParts; j++)
		{
			if (brush->parts[j].xOffset < brushMinX) brushMinX = brush->parts[j].xOffset;
			if (brush->parts[j].xOffset > brushMaxX) brushMaxX = brush->parts[j].xOffset;
			if (brush->parts[j].yOffset < brushMinY) brushMinY = brush->parts[j].yOffset;
		}

		if (UsingPaintBrush == i)
		{
			SetShader("basic");
			int outlineX0 = curPosX - (0x100);
			int outlineX1 = curPosX + (0x100) + ((brushMaxX - brushMinX + 1) << 12);
			int outlineY0 = palletteBL.y + 0x100;
			int outlineY1 = palletteBL.y + palletteSize.y - 0x100;
			DrawQuad(MAKE_COLOR(65,16,119,255),ToScreen({outlineX0,outlineY0}), ToScreen({outlineX1,outlineY0}), ToScreen({outlineX1,outlineY1}), ToScreen({outlineX0,outlineY1}));
			SetShader("basic_tex_color");
		}

		for (int j = 0; j < brush->numParts; j++)
		{
			ivec2 loc = {curPosX, (palletteBL.y + (1<<9))};
			loc.x += (brush->parts[j].xOffset - brushMinX) << 12;
			loc.y += (brush->parts[j].yOffset - brushMinY) << 12;

			game_tile *tile = &s->tiles[brush->parts[j].tileNum];
			DrawTileAtLocation(loc, tile->u0, tile->u1, tile->v0, tile->v1, Camera.pos);
		}
		curPosX += (1 << 9) + ((brushMaxX - brushMinX + 1) << 12);
	}



	ivec2 gamePos = FromScreen(MousePos);
	int tileX = gamePos.x >> 12;
	int tileY = gamePos.y >> 12;


	if (KeyboardDown[KB_O] || KeyboardDown[KB_T] || KeyboardDown[KB_E])
	{
		int x0 = tileX << 12;
		int x1 = ((tileX + 1) << 12);
		int y0 = tileY << 12;
		int y1 = ((tileY + 1) << 12);

		uint32_t col = MAKE_COLOR(255,100,0,255);
		SetShader("basic");
		DrawLine(col, col, ToScreen(x0,y0), ToScreen(x1,y0), 0x200);
		DrawLine(col, col, ToScreen(x1,y0), ToScreen(x1,y1), 0x200);
		DrawLine(col, col, ToScreen(x1,y1), ToScreen(x0,y1), 0x200);
		DrawLine(col, col, ToScreen(x0,y1), ToScreen(x0,y0), 0x200);
	}
	else
	{
		tile_paint_brush SingleTilePainter;

		tile_paint_brush *paintBrush;
		if (UsingPaintBrush < NumTilePaintBrushes[UsingTileSet])
			paintBrush = &TilePaintBrush[UsingTileSet][UsingPaintBrush];
		else
		{
			Brushes[UsingTileSet][99].numParts = 1;
			Brushes[UsingTileSet][99].parts[0].xOffset = 0;
			Brushes[UsingTileSet][99].parts[0].yOffset = 0;
			Brushes[UsingTileSet][99].parts[9].tileNum = UsingPaintBrush - NumTilePaintBrushes[UsingTileSet];

			SingleTilePainter.fromBrushes[0] = 99;
			SingleTilePainter.numBrushes = 1;
			paintBrush = &SingleTilePainter;
		}

		tile_brush *brush = &Brushes[UsingTileSet][paintBrush->fromBrushes[0]];
		for (int i = 0; i < brush->numParts; i++)
		{
			int partX = tileX + brush->parts[i].xOffset;
			int partY = tileY + brush->parts[i].yOffset;

			game_tile *tile = &s->tiles[brush->parts[i].tileNum];
			DrawTile(partX, partY, tile->u0, tile->u1, tile->v0, tile->v1, Camera.pos); 
		}
	}

	/*
	switch (CurrentSlopeType)
	{
		case EMPTY: {} break;
		case SQUARE:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1 = ((tileY + 1) << 3);

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x1,y1}, 0.5f, 1);
			DrawLine(col, col, {x1,y1}, {x0,y1}, 0.5f, 1);
			DrawLine(col, col, {x0,y1}, {x0,y0}, 0.5f, 1);
		} break;

		case SMALL_SLOPE_LEFT1:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1 = y0 + (1 << 2);

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x0,y1}, 0.5f, 1);
			DrawLine(col, col, {x0,y1}, {x0,y0}, 0.5f, 1);
		} break;

		case SMALL_SLOPE_LEFT2:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1r = (tileY << 3) + (1 << 2);
			int y1l = (tileY + 1) << 3;

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x1,y1r}, 0.5f, 1);
			DrawLine(col, col, {x1,y1r}, {x0,y1l}, 0.5f, 1);
			DrawLine(col, col, {x0,y1l}, {x0,y0}, 0.5f, 1);
		} break;

		case LARGE_SLOPE_LEFT:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1 = (tileY + 1) << 3;

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x0,y1}, 0.5f, 1);
			DrawLine(col, col, {x0,y1}, {x0,y0}, 0.5f, 1);
		} break;

		case SMALL_SLOPE_RIGHT1:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1 = y0 + (1 << 2);

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x1,y1}, 0.5f, 1);
			DrawLine(col, col, {x1,y1}, {x0,y0}, 0.5f, 1);
		} break;

		case SMALL_SLOPE_RIGHT2:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1l = (tileY << 3) + (1 << 2);
			int y1r = (tileY + 1) << 3;

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x1,y1r}, 0.5f, 1);
			DrawLine(col, col, {x1,y1r}, {x0,y1l}, 0.5f, 1);
			DrawLine(col, col, {x0,y1l}, {x0,y0}, 0.5f, 1);
		} break;

		case LARGE_SLOPE_RIGHT:
		{
			int x0 = tileX << 3;
			int x1 = ((tileX + 1) << 3);
			int y0 = tileY << 3;
			int y1 = (tileY + 1) << 3;

			uint32_t col = MAKE_COLOR(255,100,0,255);
			SetShader("basic");
			DrawLine(col, col, {x0,y0}, {x1,y0}, 0.5f, 1);
			DrawLine(col, col, {x1,y0}, {x1,y1}, 0.5f, 1);
			DrawLine(col, col, {x1,y1}, {x0,y0}, 0.5f, 1);
		} break;
	}
	*/
}

UPDATE_FUNCTION(TilePlacerUpdate)
{
	CurrentSlopeType = SQUARE;

	UsingTileSet = TS_DIRT;
	if (KeyboardPresses[KB_COMMA])
	{
		UsingPaintBrush = Max(0, (UsingPaintBrush - 1));
	}
	else if (KeyboardPresses[KB_PERIOD])
	{
		UsingPaintBrush = Min(NumTilePaintBrushes[UsingTileSet] - 1 + 64, UsingPaintBrush + 1);
	}
	/*
	if (KeyboardDown[KB_1])
		CurrentSlopeType = SMALL_SLOPE_LEFT1;
	else if (KeyboardDown[KB_2])
		CurrentSlopeType = SMALL_SLOPE_LEFT2;
	else if (KeyboardDown[KB_3])
		CurrentSlopeType = LARGE_SLOPE_LEFT;
	else if (KeyboardDown[KB_4])
		CurrentSlopeType = SMALL_SLOPE_RIGHT1;
	else if (KeyboardDown[KB_5])
		CurrentSlopeType = SMALL_SLOPE_RIGHT2;
	else if (KeyboardDown[KB_6])
		CurrentSlopeType = LARGE_SLOPE_RIGHT;
		*/

	/*if (MousePresses[0])
	{
		tx->pos.x = -1;
		tx->pos.y = -1;
	}*/

	if (MouseDown[0])
	{

		ivec2 gamePos = FromScreen(MousePos);
		int tileX = gamePos.x >> 12;
		int tileY = gamePos.y >> 12;

		if (KeyboardDown[KB_O])
		{
			TileMapI[tileY*MAP_W + tileX] = ONE_WAY;
			TileMapV[tileY*MAP_W + tileX] |= ((int)MODIFIED_TILE_TYPE << 24);
		}
		else if (KeyboardDown[KB_T])
		{
			TileMapI[tileY*MAP_W + tileX] = LADDER_TOP;
			TileMapV[tileY*MAP_W + tileX] |= ((int)MODIFIED_TILE_TYPE << 24);
		}
		else if (KeyboardDown[KB_E])
		{
			TileMapI[tileY*MAP_W + tileX] = EMPTY;
			TileMapV[tileY*MAP_W + tileX] |= ((int)MODIFIED_TILE_TYPE << 24);
		}
		else
		{
			tile_paint_brush *paintBrush = &TilePaintBrush[UsingTileSet][UsingPaintBrush];
			tile_brush *brush = &Brushes[UsingTileSet][paintBrush->fromBrushes[rand() % paintBrush->numBrushes]];
			for (int i = 0; i < brush->numParts; i++)
			{
				int partX = tileX + brush->parts[i].xOffset;
				int partY = tileY + brush->parts[i].yOffset;

				if (partX >= 0 && partY >= 0 && partX < MAP_W && partY < MAP_H)
				{
					uint8_t tileNumF = 0, tileNumB = 0;
					if (brush->parts[i].front)
						tileNumF = brush->parts[i].tileNum;
					else
						tileNumB = brush->parts[i].tileNum;
					uint8_t tileSetType = TileSetTypes[UsingTileSet].types[brush->parts[i].tileNum];
					TileMapI[partY*MAP_W + partX] = tileSetType;
					uint8_t tileFlags = TILE_IN_USE;
					if (brush->parts[i].front)
						tileFlags |= HAS_FRONT;
					else
						tileFlags |= HAS_BACK;
					TileMapV[partY*MAP_W + partX] = NewTile(tileNumF, tileNumB, UsingTileSet, tileFlags);
				}
			}
		}
	}
}

enum camera_restraint_mode
{
	CM_NONE,
	CM_SET_INITIAL_CAMERA,
	CM_SET_AREA,
	CM_ADD_AREA,
	CM_EDIT_AREA,
	CM_REMOVE_AREA,
};

global_variable camera_restraint_mode CameraRestraintMode;

void DrawCameraRestraintArea(uint32_t color, irect *area)
{
	SetShader("basic");
	uint32_t minX = area->BL.x < area->UR.x ? area->BL.x : area->UR.x;
	uint32_t minY = area->BL.y < area->UR.y ? area->BL.y : area->UR.y;
	uint32_t maxX = area->BL.x > area->UR.x ? area->BL.x : area->UR.x;
	uint32_t maxY = area->BL.y > area->UR.y ? area->BL.y : area->UR.y;

	DrawLine(color, color, ToScreen(minX, minY), ToScreen(maxX, minY), 0x200);
	DrawLine(color, color, ToScreen(maxX, minY), ToScreen(maxX, maxY), 0x200);
	DrawLine(color, color, ToScreen(maxX, maxY), ToScreen(minX, maxY), 0x200);
	DrawLine(color, color, ToScreen(minX, maxY), ToScreen(minX, minY), 0x200);
	DrawCircle(color, ToScreen(minX, minY), (5 << 9));
	DrawCircle(color, ToScreen(minX, maxY), (5 << 9));
	DrawCircle(color, ToScreen(maxX, maxY), (5 << 9));
	DrawCircle(color, ToScreen(maxX, minY), (5 << 9));
}

void DrawCameraFocus(uint32_t color, ivec2 focusPoint)
{
	SetShader("basic");
	DrawLine(color, color, ToScreen(focusPoint.x - (4 << 9), focusPoint.y), ToScreen(focusPoint.x + (4 << 9), focusPoint.y), 0x200, 1);
	DrawLine(color, color, ToScreen(focusPoint.x, focusPoint.y - (4 << 9)), ToScreen(focusPoint.x, focusPoint.y + (4 << 9)), 0x200, 1);
}

void DrawInitialCameraPos()
{
	SetShader("basic");
	uint32_t color = MAKE_COLOR(200,100,40,255);
	DrawCircle(color, ToScreen(InitialCameraPos), (6<<9));
	DrawCircle(COL_BLACK, ToScreen(InitialCameraPos), (5<<9));
	DrawLine(color, color, ToScreen(InitialCameraPos.x - (4 << 9), InitialCameraPos.y), ToScreen(InitialCameraPos.x + (4 << 9), InitialCameraPos.y), 0x200, 1);
	DrawLine(color, color, ToScreen(InitialCameraPos.x, InitialCameraPos.y - (4 << 9)), ToScreen(InitialCameraPos.x, InitialCameraPos.y + (4 << 9)), 0x200, 1);
}

void DrawCameraRegion(float zoom)
{
	uint32_t x = GameCamFocus.x;
	uint32_t y = GameCamFocus.y;
	uint32_t xL = x - (int)(Camera.width * 0.5f / zoom);
	uint32_t xR = x + (int)(Camera.width * 0.5f / zoom);
	uint32_t yB = y - (int)(Camera.height * 0.5f / zoom);
	uint32_t yT = y + (int)(Camera.height * 0.5f / zoom);

	uint32_t color = COL_WHITE;
	DrawLine(color, color, ToScreen(xL, yB), ToScreen(xR, yB), 0x200);
	DrawLine(color, color, ToScreen(xR, yB), ToScreen(xR, yT), 0x200);
	DrawLine(color, color, ToScreen(xR, yT), ToScreen(xL, yT), 0x200);
	DrawLine(color, color, ToScreen(xL, yT), ToScreen(xL, yB), 0x200);
}

UPDATE_FUNCTION(CameraRestraintEditorDraw)
{
	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));
	DrawTextGui(2, 13, "Controls");
	DrawTextGui(2, 10, "I to set initial camera");
	DrawTextGui(2, 7, "S to draw region");
	DrawTextGui(2, 4, "A to add region");
	DrawTextGui(2, 1, "R to remove region (click one handle)");

	switch (CameraRestraintMode)
	{
		case CM_NONE: DrawTextGui(40, 95, "Mode: Edit"); break;
		case CM_SET_INITIAL_CAMERA: DrawTextGui(40, 95, "Mode: Set Initial Camera"); break;
		case CM_SET_AREA: DrawTextGui(40, 95, "Mode: Set Area (will erase others)"); break;
		case CM_ADD_AREA: DrawTextGui(40, 95, "Mode: Add Area"); break;
		case CM_REMOVE_AREA: DrawTextGui(40, 95, "Mode: Remove Area"); break;
		case CM_EDIT_AREA: DrawTextGui(40, 95, "Mode: Editing"); break;
	}
}

UPDATE_FUNCTION(CameraRestraintUpdateAndDraw)
{
	local_persistent bool AddingCameraRestraintArea;
	local_persistent irect CurrentCameraRestraintArea;
	local_persistent irect *EditingArea;
	auto custom_flags = GO(custom_flags);
	if (KeyboardReleases[KB_I])
		CameraRestraintMode = CM_SET_INITIAL_CAMERA;
	if (KeyboardReleases[KB_S])
		CameraRestraintMode = CM_SET_AREA;
	if (KeyboardReleases[KB_A])
		CameraRestraintMode = CM_ADD_AREA;
	if (KeyboardReleases[KB_R])
		CameraRestraintMode = CM_REMOVE_AREA;


	for (int i = 0; i < NumCameraRestraintAreas; i++)
		DrawCameraRestraintArea(MAKE_COLOR(200,150,150,200), &CameraRestraintAreas[i]);

	int tmp = NumCameraRestraintAreas;
	NumCameraRestraintAreas = 0;
	ivec2 playerFocus = PlayerFocus();
	NumCameraRestraintAreas = tmp;

	DrawCameraFocus(MAKE_COLOR(100,200,100,200), playerFocus);
	DrawCameraFocus(MAKE_COLOR(200,100,100,200), GameCamFocus);

	DrawInitialCameraPos();

	DrawCameraRegion(2);

	switch (CameraRestraintMode)
	{
		case CM_REMOVE_AREA:
		case CM_NONE:
		{
		} break;
		
		case CM_SET_INITIAL_CAMERA:
		{
			if (MousePresses[0])
			{
				InitialCameraPos = FromScreen(MousePos);
				CameraRestraintMode = CM_NONE;
			}
		} break;

		case CM_SET_AREA:
		{
			if (MousePresses[0])
			{
				NumCameraRestraintAreas = 0;
				AddingCameraRestraintArea = true;
				CurrentCameraRestraintArea.BL = FromScreen(MousePos);
			}

			if (MouseDown[0])
				CurrentCameraRestraintArea.UR = FromScreen(MousePos);

			if (MouseReleases[0])
			{
				uint32_t minX = CurrentCameraRestraintArea.BL.x < CurrentCameraRestraintArea.UR.x ? CurrentCameraRestraintArea.BL.x : CurrentCameraRestraintArea.UR.x;
				uint32_t minY = CurrentCameraRestraintArea.BL.y < CurrentCameraRestraintArea.UR.y ? CurrentCameraRestraintArea.BL.y : CurrentCameraRestraintArea.UR.y;
				uint32_t maxX = CurrentCameraRestraintArea.BL.x > CurrentCameraRestraintArea.UR.x ? CurrentCameraRestraintArea.BL.x : CurrentCameraRestraintArea.UR.x;
				uint32_t maxY = CurrentCameraRestraintArea.BL.y > CurrentCameraRestraintArea.UR.y ? CurrentCameraRestraintArea.BL.y : CurrentCameraRestraintArea.UR.y;
				CameraRestraintAreas[0] = {{minX, minY},{maxX, maxY}};
				NumCameraRestraintAreas = 1;
				AddingCameraRestraintArea = false;
				CameraRestraintMode = CM_NONE;
			}

			if (AddingCameraRestraintArea)
				DrawCameraRestraintArea(COL_WHITE, &CurrentCameraRestraintArea);
			else
			{
				SetShader("basic");
				DrawCircle(COL_WHITE, {(float)MousePos.x, (float)MousePos.y}, (5 << 9));
			}

		} break;

		case CM_ADD_AREA:
		{
			if (MousePresses[0])
			{
				AddingCameraRestraintArea = true;
				CurrentCameraRestraintArea.BL = FromScreen(MousePos);
			}

			if (MouseDown[0])
				CurrentCameraRestraintArea.UR = FromScreen(MousePos);

			if (MouseReleases[0])
			{
				uint32_t minX = CurrentCameraRestraintArea.BL.x < CurrentCameraRestraintArea.UR.x ? CurrentCameraRestraintArea.BL.x : CurrentCameraRestraintArea.UR.x;
				uint32_t minY = CurrentCameraRestraintArea.BL.y < CurrentCameraRestraintArea.UR.y ? CurrentCameraRestraintArea.BL.y : CurrentCameraRestraintArea.UR.y;
				uint32_t maxX = CurrentCameraRestraintArea.BL.x > CurrentCameraRestraintArea.UR.x ? CurrentCameraRestraintArea.BL.x : CurrentCameraRestraintArea.UR.x;
				uint32_t maxY = CurrentCameraRestraintArea.BL.y > CurrentCameraRestraintArea.UR.y ? CurrentCameraRestraintArea.BL.y : CurrentCameraRestraintArea.UR.y;
				CameraRestraintAreas[NumCameraRestraintAreas++] = {{minX, minY},{maxX, maxY}};
				CameraRestraintMode = CM_NONE;
				AddingCameraRestraintArea = false;
			}

			if (AddingCameraRestraintArea)
				DrawCameraRestraintArea(COL_WHITE, &CurrentCameraRestraintArea);
			else
			{
				SetShader("basic");
				DrawCircle(COL_WHITE, {MousePos.x, MousePos.y}, 5);
			}

		} break;

		case CM_EDIT_AREA:
		{
			if (MouseDown[0])
				EditingArea->UR = FromScreen(MousePos);

			if (MouseReleases[0])
			{
				uint32_t minX = EditingArea->BL.x < EditingArea->UR.x ? EditingArea->BL.x : EditingArea->UR.x;
				uint32_t minY = EditingArea->BL.y < EditingArea->UR.y ? EditingArea->BL.y : EditingArea->UR.y;
				uint32_t maxX = EditingArea->BL.x > EditingArea->UR.x ? EditingArea->BL.x : EditingArea->UR.x;
				uint32_t maxY = EditingArea->BL.y > EditingArea->UR.y ? EditingArea->BL.y : EditingArea->UR.y;
				*EditingArea = {{minX, minY},{maxX, maxY}};
				CameraRestraintMode = CM_NONE;
				AddingCameraRestraintArea = false;
			}

			DrawCameraRestraintArea(COL_WHITE, EditingArea);
		} break;
	}


	if (!AddingCameraRestraintArea)
	{
		if (MousePresses[0])
		{
			ivec2 point = FromScreen(MousePos);
			point.x >>= 9;
			point.y >>= 9;
			bool removeArea = false;
			int removeIndex = -1;
			//check if we grabbed a handle
			for (int i = 0; i < NumCameraRestraintAreas; i++)
			{
				removeIndex = i;
				irect *area = &CameraRestraintAreas[i];
				ivec2 UR = {area->UR.x >> 9, area->UR.y >> 9};
				ivec2 UL = {area->BL.x >> 9, area->UR.y >> 9};
				ivec2 BR = {area->UR.x >> 9, area->BL.y >> 9};
				ivec2 BL = {area->BL.x >> 9, area->BL.y >> 9};
				if ((point.x - UR.x)*(point.x - UR.x) + (point.y - UR.y)*(point.y - UR.y) < 25)
				{
					//grabbed UR
					if (CameraRestraintMode == CM_REMOVE_AREA)
						removeArea = true;
					else
					{
						CameraRestraintMode = CM_EDIT_AREA;
						AddingCameraRestraintArea = true;
						EditingArea = area;
						break;
					}
				}
				else if ((point.x - UL.x)*(point.x - UL.x) + (point.y - UL.y)*(point.y - UL.y) < 25)
				{
					//grabbed UL
					if (CameraRestraintMode == CM_REMOVE_AREA)
						removeArea = true;
					else
					{
						CameraRestraintMode = CM_EDIT_AREA;
						AddingCameraRestraintArea = true;
						irect oldArea = *area;
						area->UR = {oldArea.BL.x, oldArea.UR.y}; //always move the UR, so swap around the positions
						area->BL = {oldArea.UR.x, oldArea.BL.y};
						EditingArea = area;
						break;
					}
				}
				else if ((point.x - BR.x)*(point.x - BR.x) + (point.y - BR.y)*(point.y - BR.y) < 25)
				{
					//grabbed BR
					if (CameraRestraintMode == CM_REMOVE_AREA)
						removeArea = true;
					else
					{
						CameraRestraintMode = CM_EDIT_AREA;
						AddingCameraRestraintArea = true;
						irect oldArea = *area;
						area->UR = {oldArea.UR.x, oldArea.BL.y}; //always move the UR, so swap around the positions
						area->BL = {oldArea.BL.x, oldArea.UR.y};
						EditingArea = area;
						break;
					}
				}
				else if ((point.x - BL.x)*(point.x - BL.x) + (point.y - BL.y)*(point.y - BL.y) < 25)
				{
					//grabbed BL
					if (CameraRestraintMode == CM_REMOVE_AREA)
						removeArea = true;
					else
					{
						CameraRestraintMode = CM_EDIT_AREA;
						AddingCameraRestraintArea = true;
						irect oldArea = *area;
						area->UR = {oldArea.BL.x, oldArea.BL.y}; //always move the UR, so swap around the positions
						area->BL = {oldArea.UR.x, oldArea.UR.y};
						EditingArea = area;
						break;
					}
				}

				if (removeArea)
					break;
			}
			if (removeArea)
			{
				for (int i = removeIndex; i < NumCameraRestraintAreas - 1; i++)
					CameraRestraintAreas[i] = CameraRestraintAreas[i+1];
				NumCameraRestraintAreas--;
				CameraRestraintMode = CM_NONE;
			}
		}
	}

}




global_variable json_data_file *SelectedPrefab;
global_variable int PlacedPrefabID;
global_variable bool PrefabPlacerInit;

UPDATE_FUNCTION(PrefabPlacer)
{
	if (!PrefabPlacerInit)
	{
		PrefabPlacerInit = true;
		PlacedPrefabID = DeserializeObject(SelectedPrefab->val);
	}
	if (KeyboardPresses[KB_ESCAPE])
	{
		RemoveObject(PlacedPrefabID);
		PrefabPlacerInit = false;
		RemoveObject(goId);
	}
	auto tx = OTH(PlacedPrefabID, transform);
	tx->pos = FromScreen(MousePos);
	tx->pos.x &= ~0x1ff;
	tx->pos.y &= ~0x1ff;

	if (MousePresses[0])
	{
		level_objects *obj = &LevelObjects[NumLevelObjects++];
		obj->prefab = SelectedPrefab->baseName;
		obj->pos = tx->pos;
		obj->objectID = PlacedPrefabID;

		//this will release the object from being held
		PrefabPlacerInit = false;
	}
}


global_variable char PrefabFolderNames[20][1024];
global_variable int NumPrefabFolders = 0;
global_variable char PrefabNames[100][64];
global_variable int NumPrefabs = 0;
local_persistent int OnPrefabFolder = -1;
local_persistent int InPrefabFolder = -1;
local_persistent int OnPrefab = -1;

UPDATE_FUNCTION(PrefabSelect)
{
	local_persistent bool init = false;
	local_persistent dir_folder prefabFolder;
	if (!init)
	{
		init = true;
		NumPrefabFolders = 0;
		prefabFolder = LoadDirectory(App.baseFolder, "prefabs");

		void *value;
		size_t iter = HashGetFirst(&prefabFolder.folders, NULL, NULL, &value);
		while (value)
		{
			dir_folder *folder = (dir_folder *)value;
			strcpy(PrefabFolderNames[NumPrefabFolders++], folder->full_path);
			HashGetNext(&prefabFolder.folders, NULL, NULL, &value, iter);
		}
		HashEndIteration(iter);

		for (int i = 0; i < NumPrefabFolders-1; i++)
		{
			for (int j = 0; j < NumPrefabFolders - 1 - i; j++)
			{
				if (strcmp(PrefabFolderNames[j], PrefabFolderNames[j+1]) > 0)
				{
					char tmp[1024];
					strcpy(tmp, PrefabFolderNames[j]);
					strcpy(PrefabFolderNames[j], PrefabFolderNames[j+1]);
					strcpy(PrefabFolderNames[j+1], tmp);
				}
			}
		}
	}

	if (OnPrefabFolder == -1 && NumPrefabFolders > 0)
		OnPrefabFolder = 0;

	if (KeyboardPresses[KB_ENTER])
	{
		if (InPrefabFolder == -1)
		{
			InPrefabFolder = OnPrefabFolder;

			dir_folder *prefabs = GetSubFolder(&prefabFolder, strstr(PrefabFolderNames[OnPrefabFolder], "prefabs") + 8);

			NumPrefabs = 0;

			void *value;
			size_t iter = HashGetFirst(&prefabs->files, NULL, NULL, &value);
			while (value)
			{
				dir_file *file = (dir_file *)value;
				if (strcmp(file->ext, "json") == 0)
					strcpy(PrefabNames[NumPrefabs++], file->basename);

				HashGetNext(&prefabs->files, NULL, NULL, &value, iter);
			}
			HashEndIteration(iter);

			for (int i = 0; i < NumPrefabs-1; i++)
			{
				for (int j = 0; j < NumPrefabs - 1 - i; j++)
				{
					if (strcmp(PrefabNames[j], PrefabNames[j+1]) > 0)
					{
						char tmp[64];
						strcpy(tmp, PrefabNames[j]);
						strcpy(PrefabNames[j], PrefabNames[j+1]);
						strcpy(PrefabNames[j+1], tmp);
					}
				}
			}

			if (NumPrefabs)
				OnPrefab = 0;
		}
		else if (InPrefabFolder == OnPrefabFolder)
		{
			if (OnPrefab >= 0)
			{
				//reset everything
				InPrefabFolder = -1;
				OnPrefabFolder = -1;
				ExitEditorMode();

				int id = AddObject(OBJ_PREFAB_PLACER);
				auto meta = OTH(id, metadata);
				meta->cmpInUse = SPECIAL_DRAW;
				auto draw = OTH(id, special_draw);
				draw->draw = PrefabPlacer;
				draw->depth = 30;

				if (PrefabPlacerInit)
				{
					PrefabPlacerInit = false;
					RemoveObject(PlacedPrefabID);
				}
				SelectedPrefab = (json_data_file *)GetFromHash(&PrefabData, PrefabNames[OnPrefab]);
			}
		}
	}

	if (InPrefabFolder != OnPrefabFolder)
	{
		if (KeyboardPresses[KB_DOWN])
			OnPrefabFolder = Min(OnPrefabFolder + 1, NumPrefabFolders - 1);
		else if (KeyboardPresses[KB_UP])
			OnPrefabFolder = Max(OnPrefabFolder - 1, 0);
	}
	else
	{
		if (KeyboardReleases[KB_B])
		{
			InPrefabFolder = -1;
		}
		if (KeyboardPresses[KB_DOWN])
			OnPrefab = Min(OnPrefab + 1, NumPrefabs - 1);
		else if (KeyboardPresses[KB_UP])
			OnPrefab = Max(OnPrefab - 1, 0);
	}


	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));
	int curY = 94;
	for (int i = 0; i < NumPrefabFolders; i++)
	{
		char buf[128];
		if (OnPrefabFolder == i && !(InPrefabFolder == OnPrefabFolder))
			sprintf(buf, ">> %s/", strstr(PrefabFolderNames[i], "prefabs") + 8);
		else
			sprintf(buf, "   %s/", strstr(PrefabFolderNames[i], "prefabs") + 8);

		DrawTextGui(2, curY, buf);
		curY -= 3;

		if (InPrefabFolder == OnPrefabFolder && InPrefabFolder == i)
		{
			for (int j = 0; j < NumPrefabs; j++)
			{
				char buf[128];
				if (OnPrefab == j)
					sprintf(buf, "  >> %s", PrefabNames[j]);
				else
					sprintf(buf, "     %s", PrefabNames[j]);

				DrawTextGui(2, curY, buf);
				curY -= 3;
			}
		}
	}

	DrawTextGui(2, 10, "Controls");
	DrawTextGui(2, 7, "Up/Down, Enter to select");
	if (InPrefabFolder == OnPrefabFolder)
		DrawTextGui(2, 4, "Press B to back out of folder");
}


UPDATE_FUNCTION(PrefabMoverEditorDraw)
{
	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));
	DrawTextGui(2, 10, "Controls");
	DrawTextGui(2, 7, "Hold D to delete");
	DrawTextGui(2, 4, "Press R to reset positions");
}


UPDATE_FUNCTION(PrefabMover)
{
	local_persistent int DraggingPrefab = -1;

	if (MouseReleases[0])
		DraggingPrefab = -1;

	if (DraggingPrefab >= 0)
	{
		level_objects *obj = &LevelObjects[DraggingPrefab];
		obj->pos = FromScreen(MousePos);
		obj->pos.x &= ~0x1ff;
		obj->pos.y &= ~0x1ff;
		auto tx = OTH(obj->objectID, transform);
		ivec2 oldPos = tx->pos;
		tx->pos = obj->pos;
		ivec2 delta = tx->pos - oldPos;
		if (OTH(obj->objectID, metadata)->cmpInUse & WAYPOINTS)
		{
			auto waypoints = OTH(obj->objectID, waypoints);
			for (int i = 0; i < waypoints->count; i++)
				waypoints->points[i] += delta;
		}
	}

	SetShader("basic");
	uint32_t color = MAKE_COLOR(200,255,200,255);

	bool deleting = KeyboardDown[KB_D];
	if (deleting)
		color = MAKE_COLOR(255,100,100,255);

	ivec2 gamePosInPixels = FromScreen(MousePos);
	gamePosInPixels.x >>= 9;
	gamePosInPixels.y >>= 9;

	bool reseting = KeyboardPresses[KB_R] && !ShiftHeld;

	for (int i = 0; i < NumLevelObjects; i++)
	{
		level_objects *obj = &LevelObjects[i];

		if (reseting)
		{
			LevelLoaderInitialized = false;
			OnLevel = LoadedLevel;
		}

		//Draw the mover control
		int x = obj->pos.x & ~0x1ff;
		int y = obj->pos.y & ~0x1ff;

		int xInPixels = x >> 9;
		int yInPixels = y >> 9;

		uint32_t highlight = MAKE_COLOR(200,255,200,100);
		if (deleting)
			highlight = MAKE_COLOR(255,100,50,100);
		if (i == DraggingPrefab)
			highlight = MAKE_COLOR(255,200,200,100);

		if (LengthSq(ivec2(xInPixels,yInPixels) - gamePosInPixels) < 25)
		{
			if (MousePresses[0])
			{
				if (deleting)
				{
					DeleteLevelObject(i);
					break;
				}
				else
					DraggingPrefab = i;
			}

			DrawCircle(highlight, ToScreen(x, y), (6<<9));
		}

		if (deleting)
		{
			DrawLine(color, color, ToScreen(x - (3<<9), y + (3<<9)), ToScreen(x + (3<<9), y - (3<<9)), 0x100);
			DrawLine(color, color, ToScreen(x + (3<<9), y + (3<<9)), ToScreen(x - (3<<9), y - (3<<9)), 0x100);
		}
		else
		{
			DrawCircle(color, ToScreen(x, y), 0x400);

			DrawLine(color, color, ToScreen(x, y - (5<<9)), ToScreen(x, y + (5<<9)), 0x100);
			DrawLine(color, color, ToScreen(x - (5<<9), y), ToScreen(x + (5<<9), y), 0x100);

			//top arrow
			DrawLine(color, color, ToScreen(x, y + (5<<9)), ToScreen(x - (2<<9), y + (3<<9)), 0x100);
			DrawLine(color, color, ToScreen(x, y + (5<<9)), ToScreen(x + (2<<9), y + (3<<9)), 0x100);

			//bottom arrow
			DrawLine(color, color, ToScreen(x, y - (5<<9)), ToScreen(x - (2<<9), y - (3<<9)), 0x100);
			DrawLine(color, color, ToScreen(x, y - (5<<9)), ToScreen(x + (2<<9), y - (3<<9)), 0x100);

			//left arrow
			DrawLine(color, color, ToScreen(x - (5<<9), y), ToScreen(x - (3<<9), y - (2<<9)), 0x100);
			DrawLine(color, color, ToScreen(x - (5<<9), y), ToScreen(x - (3<<9), y + (2<<9)), 0x100);

			//right arrow
			DrawLine(color, color, ToScreen(x + (5<<9), y), ToScreen(x + (3<<9), y - (2<<9)), 0x100);
			DrawLine(color, color, ToScreen(x + (5<<9), y), ToScreen(x + (3<<9), y + (2<<9)), 0x100);
		}
	}
}

global_variable char StringStorage[512];
global_variable int EditingStringForObject = -1;

UPDATE_FUNCTION(EditObjectsEditorDraw)
{
	if (EditingStringForObject > -1)
	{
		SetShader("basic_tex_color");
		SetFontAsPercentageOfScreen("JackInput", 2.4f);
		SetFontColor(MAKE_COLOR(255,100,0,255));
		DrawTextGui(2, 13, "String:");
		DrawTextGui(8, 13, StringStorage);
	}
}

UPDATE_FUNCTION(EditObjectsDraw)
{
	local_persistent int EditingObject = -1;
	local_persistent int EditingWaypoint = -1;

	if (!MouseDown[0])
	{
		if (EditingObject != -1)
		{
			int x = 0;
		}
		EditingObject = -1;
		EditingWaypoint = -1;
	}

	SetShader("basic");
	uint32_t baseColor = MAKE_COLOR(200,0,0,255);
	uint32_t highlightColor = MAKE_COLOR(200,100,40,255);
	uint32_t endOfChainColor = MAKE_COLOR(0,200,0,255);
	uint32_t addingColor = MAKE_COLOR(200,0,200,255);
	uint32_t addingSymbolColor = MAKE_COLOR(200,200,200,255);
	
	ivec2 gamePosInPixels = FromScreen(MousePos);
	gamePosInPixels.x >>= 9;
	gamePosInPixels.y >>= 9;

	if (EditingObject > -1 && EditingWaypoint > -1)
	{
		level_objects *obj = &LevelObjects[EditingObject];
		ivec2 *point = &OTH(obj->objectID, waypoints)->points[EditingWaypoint];
		*point = ivec2(gamePosInPixels.x << 9, gamePosInPixels.y << 9);
	}

	if (EditingStringForObject > -1)
	{
		level_objects *obj = &LevelObjects[EditingStringForObject];
		auto str = OTH(obj->objectID, string_storage);
		if (KeyboardDown[KB_ESCAPE] || KeyboardDown[KB_ENTER])
		{
			EditingStringForObject = -1;
			str->string = PUSH_ARRAY(&Arena, char, strlen(StringStorage) + 1);
			strcpy(str->string, StringStorage);
		}

		int len = strlen(StringStorage);
		for (int i = KB_A; i <= KB_Z; i++)
		{
			if (KeyboardPresses[i])
			{
				if (KeyboardDown[KB_L_SHIFT] || KeyboardDown[KB_R_SHIFT])
				{
					StringStorage[len] = 'A' + (i - KB_A);
					StringStorage[++len] = '\0';
				}
				else
				{
					StringStorage[len] = 'a' + (i - KB_A);
					StringStorage[++len] = '\0';
				}
			}
		}
		for (int i = KB_0; i <= KB_9; i++)
		{
			if (KeyboardPresses[i])
			{
				const char *numberKeySymbols = ")!@#$%^&*(";
				if (KeyboardDown[KB_L_SHIFT] || KeyboardDown[KB_R_SHIFT])
					StringStorage[len] = numberKeySymbols[i - KB_0];
				else
					StringStorage[len] = '0' + (i - KB_0);

				StringStorage[++len] = '\0';
			}
		}
		if (KeyboardPresses[KB_PERIOD])
		{
			StringStorage[len] = '.';
			StringStorage[++len] = '\0';
		}
		if (KeyboardPresses[KB_COMMA])
		{
			StringStorage[len] = ',';
			StringStorage[++len] = '\0';
		}
		if (KeyboardPresses[KB_COMMA])
		{
			StringStorage[len] = ',';
			StringStorage[++len] = '\0';
		}
		if (KeyboardPresses[KB_SPACE])
		{
			StringStorage[len] = ' ';
			StringStorage[++len] = '\0';
		}

		if (KeyboardPresses[KB_DELETE])
		{
			StringStorage[len-1] = '\0';
		}
	}

	for (int i = 0; i < NumLevelObjects; i++)
	{
		level_objects *obj = &LevelObjects[i];
		auto metadata = OTH(obj->objectID, metadata);
		auto tx = OTH(obj->objectID, transform);
		if (metadata->cmpInUse & STRING_STORAGE)
		{
			auto str = OTH(obj->objectID, string_storage);
			int x = tx->pos.x & ~0x1ff;
			int y = tx->pos.y & ~0x1ff;

			int xInPixels = x >> 9;
			int yInPixels = y >> 9;

			DrawCircle(baseColor, ToScreen(x, y), 2<<9);

			if (LengthSq(ivec2(xInPixels,yInPixels) - gamePosInPixels) < 25)
			{
				DrawCircle(endOfChainColor, ToScreen(x, y), (2<<9));
				if (MousePresses[0])
				{
					if (str->string)
						strcpy(StringStorage, str->string);
					EditingStringForObject = i;
				}
			}
		}

		if (metadata->cmpInUse & WAYPOINTS)
		{
			auto waypoints = OTH(obj->objectID, waypoints);

			ivec2 positions[4];
			positions[0] = tx->pos;
			positions[1] = waypoints->points[0];
			positions[2] = waypoints->points[1];
			positions[3] = waypoints->points[2];

			for (int j = 0; j < waypoints->count + 1; j++)
			{
				//don't show the control for the body of the object if we already have a waypoint
				if (waypoints->count > 0 && j == 0)
					continue;

				ivec2 wpPos = positions[j];

				//Draw the control
				int x = wpPos.x & ~0x1ff;
				int y = wpPos.y & ~0x1ff;

				int xInPixels = x >> 9;
				int yInPixels = y >> 9;

				DrawCircle(baseColor, ToScreen(x, y), 2<<9);

				//if this is the last in the chain, we can add to it
				bool canAdd = false;
				if (j == waypoints->count && waypoints->count < 3)
				{
					canAdd = true;
					DrawCircle(endOfChainColor, ToScreen(x, y), (2<<9));
					if (KeyboardDown[KB_A])
					{
						DrawCircle(addingColor, ToScreen(x, y), (2<<9));
						DrawLine(addingSymbolColor, addingSymbolColor, ToScreen(x - (3<<9), y), ToScreen(x + (3<<9), y), 0x100);
						DrawLine(addingSymbolColor, addingSymbolColor, ToScreen(x, y + (3<<9)), ToScreen(x, y - (3<<9)), 0x100);
					}
				}

				if (LengthSq(ivec2(xInPixels,yInPixels) - gamePosInPixels) < 25)
				{
					DrawCircle(highlightColor, ToScreen(x, y), (2<<9));
					if (KeyboardDown[KB_A] && MousePresses[0] && canAdd)
					{
						EditingObject = i;
						EditingWaypoint = waypoints->count++;
						break;
					}
					else if (MousePresses[0] && j > 0 && KeyboardDown[KB_D])
					{
						for (int k = j-1; k < waypoints->count-1; k++)
						{
							waypoints->points[k] = waypoints->points[k+1];
						}
						waypoints->count--;
						break;
					}
					else if (MousePresses[0] && j > 0)
					{
						EditingObject = i;
						EditingWaypoint = j-1;
						break;
					}

					if (j > 0 && KeyboardDown[KB_D])
					{
						DrawCircle(baseColor, ToScreen(x, y), (2<<9));
						DrawLine(addingSymbolColor, addingSymbolColor, ToScreen(x - (3<<9), y + (3<<9)), ToScreen(x + (3<<9), y - (3<<9)), 0x100);
						DrawLine(addingSymbolColor, addingSymbolColor, ToScreen(x + (3<<9), y + (3<<9)), ToScreen(x - (3<<9), y - (3<<9)), 0x100);
					}
				}
			}
			if (waypoints->count > 0)
				DrawLine(addingSymbolColor, addingSymbolColor, ToScreen(tx->pos), ToScreen(waypoints->points[0]), 0x100);

			for (int j = 0; j < waypoints->count - 1; j++)
				DrawLine(addingSymbolColor, addingSymbolColor, ToScreen(waypoints->points[j]), ToScreen(waypoints->points[j+1]), 0x100);
		}
	}

}


UPDATE_FUNCTION(ModeSelectorEditorDraw)
{
	SetShader("basic_tex_color");
	SetFontAsPercentageOfScreen("JackInput", 2.4f);
	SetFontColor(MAKE_COLOR(255,100,0,255));
	if (EditorMode == NONE)
	{
		DrawTextGui(2, 94, "Modes:");
		DrawTextGui(2, 91, "Shift+T '%s'", editor_mode_names[1]);
		DrawTextGui(2, 88, "Shift+C '%s'", editor_mode_names[2]);
		DrawTextGui(2, 85, "Shift+P '%s'", editor_mode_names[3]);
		DrawTextGui(2, 82, "Shift+M '%s'", editor_mode_names[4]);
		DrawTextGui(2, 79, "Shift+R '%s'", editor_mode_names[5]);
		DrawTextGui(2, 76, "Shift+L '%s'", editor_mode_names[6]);
		DrawTextGui(2, 73, "Shift+O '%s'", editor_mode_names[7]);
		DrawTextGui(2, 70, "Shift+D '%s'", editor_mode_names[8]);
	}
	else
	{
		DrawTextGui(40, 97.5f, "ESC to exit '%s'", editor_mode_names[EditorMode]);
	}
}

UPDATE_FUNCTION(ModeSelector)
{
	if (KeyboardReleases[KB_ESCAPE])
	{
		ExitEditorMode();
	}
	else if (ShiftHeld && KeyboardPresses[KB_T] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = FreelyMoveableFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = TILE_PLACE;
		TilePlaceID = AddObject(OBJ_TILE_PLACER);
		auto meta = OTH(TilePlaceID, metadata);
		auto update = OTH(TilePlaceID, update);
		auto draw = OTH(TilePlaceID, special_draw);
		auto editor_draw = OTH(TilePlaceID, draw_editor_gui);
		meta->cmpInUse = UPDATE | TRANSFORM | SPECIAL_DRAW | DRAW_EDITOR_GUI;
		update->update = TilePlacerUpdate;
		draw->draw = TilePlacerDraw;
		draw->depth = 30;
		editor_draw->draw = TilePlacerEditorDraw;
	}
	else if (ShiftHeld && KeyboardPresses[KB_C] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = FreelyMoveableFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = CAMERA_RESTRAINT;

		Camera.width *= 2;
		Camera.height *= 2;

		CameraRestraintID = AddObject(OBJ_CAMERA_RESTRAINT_PLACER);
		auto meta = OTH(CameraRestraintID, metadata);
		auto special_draw = OTH(CameraRestraintID, special_draw);
		auto editor_gui = OTH(CameraRestraintID, draw_editor_gui);
		meta->cmpInUse = SPECIAL_DRAW | CUSTOM_FLAGS | DRAW_EDITOR_GUI;
		special_draw->draw = CameraRestraintUpdateAndDraw;
		special_draw->depth = 30;
		editor_gui->draw = CameraRestraintEditorDraw;
	}
	else if (ShiftHeld && KeyboardPresses[KB_P] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = PlayerFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = DISPLAY_PREFAB_MENU;
		PrefabSelectID = AddObject(OBJ_PREFAB_SELECTOR);
		auto meta = OTH(PrefabSelectID, metadata);
		auto draw = OTH(PrefabSelectID, draw_editor_gui);
		meta->cmpInUse = DRAW_EDITOR_GUI;
		draw->draw = PrefabSelect;
	}
	else if (ShiftHeld && KeyboardPresses[KB_M] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = FreelyMoveableFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = MOVE_PREFABS;
		MovePrefabID = AddObject(OBJ_PREFAB_MOVER);
		auto meta = OTH(MovePrefabID, metadata);
		auto draw = OTH(MovePrefabID, special_draw);
		auto editor_draw = OTH(MovePrefabID, draw_editor_gui);
		meta->cmpInUse = SPECIAL_DRAW | DRAW_EDITOR_GUI;
		draw->draw = PrefabMover;
		draw->depth = 200;
		editor_draw->draw = PrefabMoverEditorDraw;
	}
	else if (ShiftHeld && KeyboardPresses[KB_R] && EditorMode == NONE)
	{
		SerializeLevel(&SavedLevel, LevelName);
		ExitEditorMode();
		FocusFunction = PlayerFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = RUNNING_GAME;
		SendingGameUpdateEvents = true;
	}
	else if (ShiftHeld && KeyboardPresses[KB_L] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = PlayerFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = SELECTING_LEVEL;
		LevelSelectID = AddObject(OBJ_LEVEL_SELECTOR);
		auto meta = OTH(LevelSelectID, metadata);
		auto draw = OTH(LevelSelectID, draw_editor_gui);
		meta->cmpInUse = DRAW_EDITOR_GUI;
		draw->draw = LevelSelect;
	}
	else if (ShiftHeld && KeyboardPresses[KB_O] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = FreelyMoveableFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = EDIT_OBJECTS;
		EditObjectID = AddObject(OBJ_EDIT_OBJECTS);
		auto meta = OTH(EditObjectID, metadata);
		auto draw = OTH(EditObjectID, special_draw);
		auto editor_draw = OTH(EditObjectID, draw_editor_gui);
		meta->cmpInUse = SPECIAL_DRAW | DRAW_EDITOR_GUI;
		draw->draw = EditObjectsDraw;
		draw->depth = 200;
		editor_draw->draw = EditObjectsEditorDraw;
	}
	else if (ShiftHeld && KeyboardPresses[KB_D] && EditorMode == NONE)
	{
		ExitEditorMode();
		FocusFunction = PlayerFocus;
		CameraUpdateFunction = CameraLerpFunction;
		EditorMode = DEBUG_FUNCTIONS;
		DebugFunctionsID = AddObject(OBJ_DEBUG_FUNCTIONS);
		auto meta = OTH(DebugFunctionsID, metadata);
		auto draw = OTH(DebugFunctionsID, draw_editor_gui);
		meta->cmpInUse = DRAW_EDITOR_GUI;
		draw->draw = DebugFunctionsUpdateAndDraw;
	}
}

#define ADD_GAME_FUNCTION(Name) GameFunctions[NumGameFunctions++] = {#Name, Name}

void InitGameFunctions()
{
	ADD_GAME_FUNCTION(PlayerDraw);
	ADD_GAME_FUNCTION(PlayerUpdate);
	ADD_GAME_FUNCTION(PlatformUpdate);
	ADD_GAME_FUNCTION(StandinUpdate);
	ADD_GAME_FUNCTION(JumperUpdate);
	ADD_GAME_FUNCTION(SwooperUpdate);
	ADD_GAME_FUNCTION(SwooperSaveAndLoad);
	ADD_GAME_FUNCTION(DoorSaveAndLoad);
}

