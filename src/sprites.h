#pragma once

#include "collision.h"

struct sprite_atlas
{
	texture *tex;
	uint8_t *rgba_tex_data;
	rectangle_atlas atlas;
};

struct sprite_pack
{
	int num_atlases;
	int current_atlas;
	sprite_atlas atlas[10];
};

struct poi
{
	char name[16];
	glm::ivec2 location; //relative to origin
};

struct game_sprite
{
	int atlas_index;
	sub_rectangle *tex_rect;
	float u0, v0, u1, v1;
	glm::vec2 origin;
	int numColliders;
	CollisionObject colliders[10];
	poi POIs[16];
	int numPOIs;
};

struct game_tile
{
	sub_rectangle *tex_rect;
	float u0, v0, u1, v1;
};

struct game_tile_sprites
{
	int atlas_index;
	game_tile tiles[255];
	int numTiles;
};

poi *GetPOI(game_sprite *s, char *name)
{
	for (int i = 0; i < s->numPOIs; i++)
	{
		poi *POI = &s->POIs[i];
		if (strcmp(POI->name, name) == 0)
		{
			return POI;
		}
	}
	return NULL;
}


void AddAtlasToSpritePack(sprite_pack *pack);
game_sprite *AddSpriteToAvailableAtlas(sprite_pack *pack, dir_file *file, bool render_immediately);
game_tile_sprites *AddTileSpritesToAvailableAtlas(sprite_pack *pack, dir_file *file, bool render_immediately);
