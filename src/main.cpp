#include "dan_engine.h"

global_variable application App;
global_variable renderer Rend;

#define MAP_W 1024
#define MAP_H 1024
global_variable uint8_t			TileMapI[MAP_W*MAP_H]; //interactive
global_variable uint32_t		TileMapV[MAP_W*MAP_H]; //visual

struct level_objects
{
	char *prefab;
	ivec2 pos;
	int layer;
	int objectID;
};

global_variable level_objects LevelObjects[1000];
global_variable int NumLevelObjects;

enum persistent_data_type
{
	DATA_INT,
	DATA_FLOAT,
	DATA_STRING,
	DATA_GENERIC,
};

struct persistent_data
{
	union
	{
		int i;
		float f;
		char *str;
		void *data;
	};
};

global_variable generic_hash PersistentData;

#include "components.h"
#include "actions.h"

void DeleteLevelObject(int i)
{
	assert(i < NumLevelObjects);
	RemoveObject(LevelObjects[i].objectID);
	if (i < NumLevelObjects - 1)
		LevelObjects[i] = LevelObjects[NumLevelObjects-1];
	NumLevelObjects--;
}

struct GameCamera
{
	glm::ivec2 pos;
	int width;
	int height;
	float aspect; //w/h
	float rotation; //not used currently
};
global_variable GameCamera		Camera;

void SerializeLevel(bool *savedLevel, char *levelName = NULL);
#include "systems.cpp"
#include "updates.cpp"
#include "actions.cpp"
#include "serialization.h"

#include "dan_engine.cpp"

struct draw_object
{
	int depth;
	int goId;
	union
	{
		struct
		{
			cmp_sprite *sprite;
			cmp_transform *tx;
		};
		update_function *draw;
	};
};

int main(int arg_count, char **args)
{
	srand(time(NULL));
	ApplicationInit("Game Window", {274, 154}, 4);
	RendererInit();

	SetupGameInput();
	SetupGameData();
	SfxInitialize();

	//InitGame();

	Camera.pos = {4000 << 9, 4000 << 9};
	Camera.width = (int)App.gameWindowSize.x << 9;
	Camera.height = (int)App.gameWindowSize.y << 9;
	Camera.aspect = Camera.width / (float)Camera.height;

	{
		InitGameFunctions();
		InitTiles();
		InitGlobalActions();
	}

	{
		int goId = AddObject(OBJ_MODE_SELECTOR);
		auto meta = GO(metadata);
		auto update = GO(update);
		auto editor_draw = GO(draw_editor_gui);
		meta->cmpInUse = UPDATE | DRAW_EDITOR_GUI;
		InitObject(goId);
		update->update = ModeSelector;
		editor_draw->draw = ModeSelectorEditorDraw;
	}

	{
		int goId = AddObject(OBJ_GAME_CAMERA);
		auto meta = GO(metadata);
		meta->cmpInUse = UPDATE;
		auto update = GO(update);
		InitObject(goId);
		update->update = CameraUpdate;
	}

	{
		int goId = AddObject(OBJ_LEVEL_LOADER);
		auto meta = GO(metadata);
		auto update = GO(update);
		meta->cmpInUse = UPDATE;
		InitObject(goId);
		update->update = LevelLoader;
	}

	{
		int goId = AddObject(OBJ_LEVEL_SAVER);
		auto meta = GO(metadata);
		auto update = GO(update);
		meta->cmpInUse = UPDATE;
		InitObject(goId);
		update->update = LevelSaver;
	}

	/*
	{
		int goId = AddObject(OBJ_MAGIC_BULLET_TRAIL_DRAWER);
		auto meta = GO(metadata);
		auto special_draw = GO(special_draw);
		meta->cmpInUse = SPECIAL_DRAW;
		InitObject(goId);
		special_draw->draw = MagicBulletTrailDrawer;
		special_draw->depth = 11;
	}

	{
		int goId = AddObject(OBJ_BASIC_BULLET_DRAWER);
		auto meta = GO(metadata);
		auto special_draw = GO(special_draw);
		meta->cmpInUse = SPECIAL_DRAW;
		InitObject(goId);
		special_draw->draw = BasicBulletUpdateAndDraw;
		special_draw->depth = 11;
	}
	*/

	{
		int goId = AddObject(OBJ_TILE_DRAWER);
		auto meta = GO(metadata);
		auto flag = GO(custom_flags);
		auto special_draw = GO(special_draw);
		meta->cmpInUse = SPECIAL_DRAW | CUSTOM_FLAGS;
		InitObject(goId);
		special_draw->draw = TileDrawer;
		special_draw->depth = 10;
		flag->bits = TILE_DRAWER_DRAW_FRONT;
	}

	{
		int goId = AddObject(OBJ_TILE_DRAWER);
		auto meta = GO(metadata);
		auto flag = GO(custom_flags);
		auto special_draw = GO(special_draw);
		meta->cmpInUse = SPECIAL_DRAW | CUSTOM_FLAGS;
		InitObject(goId);
		special_draw->draw = TileDrawer;
		special_draw->depth = 4;
		flag->bits = TILE_DRAWER_DRAW_BACK;
	}

	/*{
		int goId = AddObject(OBJ_RAIN_GENERATOR);
		auto meta = GO(metadata);
		auto special_draw = GO(special_draw);
		meta->cmpInUse = SPECIAL_DRAW;
		special_draw->draw = RainUpdateAndDraw;
	}*/

	{
		int goId = AddObject(OBJ_DEBUG_DISPLAY);
		auto meta = GO(metadata);
		auto draw = GO(draw_editor_gui);
		meta->cmpInUse = DRAW_EDITOR_GUI;
		InitObject(goId);
		draw->draw = DisplayDebugMessage;
	}


	CreateFrameBuffer("game_buffer", App.gameWindowSize.x * 2, App.gameWindowSize.y * 2, true, true, true);

	GameGuiVP = glm::ortho(0.0f, (float)(Camera.width >> 8), 0.0f, (float)(Camera.height >> 8), 150000.0f, -150000.0f);
	GetFrameBufferWindowProjection("game_buffer", &GameWindowProj);


	bool running = true;
	float deltaTime = 0.01666f;

	while(true)
	{
		GameVP = glm::ortho(-(float)(Camera.width >> 1), (float)(Camera.width >> 1), -(float)(Camera.height >> 1), (float)(Camera.height >> 1) , 150000.0f, -150000.0f);

		if (!ProcessEvents())
			break;

		App.currentTime++;

		SfxUpdate();
		UpdateGameSounds(deltaTime);

		Prepare2DRender();
		
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);

		SetFrameBuffer("game_buffer");
		SetViewFromMatrix(&GameVP);
		ClearScreen(0.15f, 0.15f,0.15f,1, true);

		//Process game objects
		{
			//Process actions first
			ProcessActions(0.016f);

			//check objects that might be on moveable platforms, or pushed by a platform
			if (SendingGameUpdateEvents)
			{
				for (int i = 0; i < NumGameObjects; i++)
				{
					game_object *go = &GameObjects[i];
					if (!go->inUse)
						continue;

					auto meta = &go->metadata;
					if ((meta->cmpInUse & RIDES_PLATFORMS) && (meta->cmpInUse & TRANSFORM))
					{
						bool ridingPlatform = false;
						auto rider = &go->rides_platforms;
						auto tx = &go->transform;
						for (int j = 0; j < NumGameObjects; j++)
						{
							game_object *oth = &GameObjects[j];
							if (!oth->inUse)
								continue;

							auto platformMeta = &oth->metadata;
							if ((platformMeta->cmpInUse & MOVING_PLATFORM) && (platformMeta->cmpInUse & TRANSFORM) && (platformMeta->cmpInUse & PHYSICS))
							{
								auto platform = &oth->moving_platform;
								auto platformTx = &oth->transform;
								auto platformPhys = &oth->physics;

								int plT = platformTx->pos.y + platform->ur.y + platformPhys->vel.y;
								int plB = platformTx->pos.y + platform->bl.y + platformPhys->vel.y;
								int plL = platformTx->pos.x + platform->bl.x + platformPhys->vel.x;
								int plR = platformTx->pos.x + platform->ur.x + platformPhys->vel.x;
								int riderT = tx->pos.y + rider->size.y;
								int riderB = tx->pos.y - 0x200;
								int riderL = tx->pos.x - (rider->size.x >> 1);
								int riderR = tx->pos.x + (rider->size.x >> 1);

								if (riderT > plB && riderB < plT && riderL < plR && riderR > plL)
								{
									ridingPlatform = true;
									if (rider->platformID == -1)
									{
										//store the platform information into the rider
										auto riderPhy = &go->physics;
										riderPhy->vel -= platformPhys->vel;
										rider->platformID = j;
									}
									tx->pos += platformPhys->vel;
									if (platformPhys->vel.y > 0)
									{
										rider->pushedV = platformPhys->vel.y;
									}
									rider->lastVel = platformPhys->vel;
								}
								/*
								ivec2 centerFoot = {tx->pos.x, tx->pos.y - 0x200};

								if (PointInAABB(centerFoot, {platformTx->pos.x + platform->bl.x, 
											platformTx->pos.y + platform->bl.y}, 
											{platformTx->pos.x + platform->ur.x, 
											platformTx->pos.y + platform->ur.y}))
								{
									ridingPlatform = true;
									if (rider->platformID == -1)
									{
										//store the platform information into the rider
										auto riderPhy = &GameComponents.physics[i];
										riderPhy->vel -= platformPhys->vel;
										rider->platformID = GameComponents.idIndex[j];
									}
									tx->pos += platformPhys->vel;
									if (platformPhys->vel.y > 0)
									{
										rider->pushedV = platformPhys->vel.y;
									}
									rider->lastVel = platformPhys->vel;
								}
								*/

								plT = platformTx->pos.y + platform->ur.y + platformPhys->vel.y;
								plB = platformTx->pos.y + platform->bl.y + platformPhys->vel.y;
								plL = platformTx->pos.x + platform->bl.x + platformPhys->vel.x;
								plR = platformTx->pos.x + platform->ur.x + platformPhys->vel.x;
								riderT = tx->pos.y + rider->size.y;
								riderB = tx->pos.y;
								riderL = tx->pos.x - (rider->size.x >> 1);
								riderR = tx->pos.x + (rider->size.x >> 1);

								if (riderT > plB && riderB < plT && riderL < plR && riderR > plL)
								{
									int overlapT = -(riderT - plB);
									int overlapB = (plT - riderB);
									int overlapV = 0;
									if (Abs(overlapT) < Abs(overlapB))
										overlapV = overlapT;
									else
										overlapV = overlapB;

									int overlapH = 0;
									if (platformPhys->vel.x < 0)
										overlapH = -(riderR - plL);
									else if (platformPhys->vel.x > 0)
										overlapH = (plR - riderL);

									if (Abs(overlapH) < Abs(overlapV))
									{
										tx->pos.x += overlapH;
										rider->pushedH = overlapH;
									}
									else
									{
										tx->pos.y += overlapV;
										rider->pushedV = overlapV;
									}
								}
							}
						}
						if (!ridingPlatform && rider->platformID != -1)
						{
							//add the velocity to the rider's physics once separated
							auto riderPhy = &go->physics;
							riderPhy->vel += rider->lastVel;
							rider->platformID = -1;
						}
					}
				}
			}

			for (int i = 0; i < NumGameObjects; i++)
			{
				game_object *go = &GameObjects[i];
				if (!go->inUse)
					continue;

				auto meta = &go->metadata;

				//update all transforms
				if (!(meta->flags & GAME_OBJECT) || (SendingGameUpdateEvents && (meta->flags & GAME_OBJECT)))
				{
					if ((meta->cmpInUse & PHYSICS) && (meta->cmpInUse & TRANSFORM))
					{
						auto tx = &go->transform;
						auto phys = &go->physics;

						phys->vel += phys->accel;
						tx->pos += phys->vel;
					}
				}
			}

			
			//Check for all collisions
			for (int i = 0; i < NumGameObjects; i++)
			{
				game_object *go = &GameObjects[i];
				if (!go->inUse)
					continue;

				auto meta = &go->metadata;
				if ((meta->cmpInUse & COLLIDER) && (meta->cmpInUse & TRANSFORM))
					CollisionUpdate(i);
			}


			draw_object objectsToDraw[10000];
			int numObjectsToDraw = 0;

			for (int i = 0; i < NumGameObjects; i++)
			{
				game_object *go = &GameObjects[i];
				if (!go->inUse)
					continue;

				auto meta = &go->metadata;

				if (!(meta->flags & GAME_OBJECT) || (SendingGameUpdateEvents && (meta->flags & GAME_OBJECT)))
				{
					if ((meta->cmpInUse & ANIM) && (meta->cmpInUse & SPRITE))
						AnimationUpdate(i);

					if (meta->cmpInUse & UPDATE)
					{
						auto update = &go->update;
						update->update(i);
					}
				}

				if (meta->cmpInUse & SPECIAL_DRAW)
				{
					auto draw = &go->special_draw;
					draw_object *obj = &objectsToDraw[numObjectsToDraw++];
					obj->depth = draw->depth;
					obj->draw = draw->draw;
					obj->goId = i;
				}
				else if ((meta->cmpInUse & TRANSFORM) && (meta->cmpInUse & SPRITE))
				{
					auto sprite = &go->sprite;
					auto tx = &go->transform;

					draw_object *obj = &objectsToDraw[numObjectsToDraw++];
					obj->depth = sprite->depth;
					obj->sprite = sprite;
					obj->tx = tx;
					obj->goId = -1;
				}
			}

			for (int i = 0; i < numObjectsToDraw-1; i++)
			{
				for (int j = 0; j < numObjectsToDraw-i-1; j++)
				{
					if (objectsToDraw[j].depth > objectsToDraw[j+1].depth)
					{
						draw_object tmp = objectsToDraw[j];
						objectsToDraw[j] = objectsToDraw[j+1];
						objectsToDraw[j+1] = tmp;
					}
				}
			}

			for (int i = 0; i < numObjectsToDraw; i++)
			{
				draw_object *obj = &objectsToDraw[i];
				if (obj->goId >= 0)
				{
					obj->draw(obj->goId);
				}
				else
				{
					SetShader("basic_tex_color");
                    DrawSprite(obj->sprite->name, obj->sprite->color, ToScreen(obj->tx->pos, obj->tx->layer), obj->tx->rot, {obj->tx->scale.x * 0x200, obj->tx->scale.y * 0x200});
				}
			}


			SetViewFromMatrix(&GameGuiVP);

			for (int i = 0; i < NumGameObjects; i++)
			{
				game_object *go = &GameObjects[i];
				if (!go->inUse)
					continue;

				auto meta = &go->metadata;
				if (meta->cmpInUse & DRAW_GAME_GUI)
				{
					auto draw = OTH(i, draw_game_gui);
					draw->draw(i);
				}
			}

			ResetFrameBuffer();
			SetView(0, 0, 1);
			SetShader("basic_tex_color");
			SetTextureFromFrameBuffer("game_buffer");
			DrawTexture(COL_WHITE, 0, 0, 4, 4);

			SetViewFromMatrix(&GuiVP);

			for (int i = 0; i < NumGameObjects; i++)
			{
				game_object *go = &GameObjects[i];
				if (!go->inUse)
					continue;

				auto meta = &go->metadata;
				if (meta->cmpInUse & DRAW_EDITOR_GUI)
				{
					auto draw = OTH(i, draw_editor_gui);
					draw->draw(i);
				}
			}


		}
		ProcessObjectRemovals();

		Process2DRenderList();

		ShowFrame();
		ReloadDynamicData();
	}

	ApplicationShutdown();
	return 0;
}
