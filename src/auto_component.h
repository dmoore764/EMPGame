enum object_type
{
    OBJ_NONE,
    OBJ_DIALOGUE_BOX,
    OBJ_PLAYER,
    OBJ_HEALTH_BAR,
    OBJ_KNOCKBACK,
    OBJ_SWORD_SWIPE,
    OBJ_DUST,
    OBJ_CRYSTAL_GENERATOR,
    OBJ_GENERATOR_DISPLAY,
    OBJ_UPGRADE_PANEL,
    OBJ_CRYSTAL,
    OBJ_PLATFORM,
    OBJ_MAGIC_BULLET,
    OBJ_MAGIC_BULLET_TRAIL_DRAWER,
    OBJ_BASIC_BULLET_DRAWER,
    OBJ_GAME_CAMERA,
    OBJ_ROCK,
    OBJ_DOOR,
    OBJ_RAIN_GENERATOR,
    OBJ_LEVEL_LOADER,
    OBJ_LEVEL_SAVER,
    OBJ_DEBUG_DISPLAY,
    OBJ_TILE_DRAWER,
    OBJ_MODE_SELECTOR,
    OBJ_TILE_PLACER,
    OBJ_CAMERA_RESTRAINT_PLACER,
    OBJ_PREFAB_SELECTOR,
    OBJ_PREFAB_MOVER,
    OBJ_PREFAB_PLACER,
    OBJ_LEVEL_SELECTOR,
    OBJ_EDIT_OBJECTS,
    OBJ_DEBUG_FUNCTIONS,
    OBJ_STANDIN,
    OBJ_ENEMY_JUMPER,
    OBJ_ENEMY_PROJECTILE,
    OBJ_ENEMY_SWORD_SWIPE,
    OBJ_ENEMY_SWOOPER,
};


enum component_type {
    METADATA,
    DRAW_EDITOR_GUI = 0x1,
    DRAW_GAME_GUI = 0x2,
    PARENT = 0x4,
    PHYSICS = 0x8,
    RIDES_PLATFORMS = 0x10,
    SAVE_AND_LOAD = 0x20,
    STRING_STORAGE = 0x40,
    TRANSFORM = 0x80,
    UPDATE = 0x100,
    WAYPOINTS = 0x200,
    ANIM = 0x400,
    CUSTOM_FLAGS = 0x800,
    COLLIDER = 0x1000,
    SPECIAL_DRAW = 0x2000,
    DIALOG = 0x4000,
    COUNTERS = 0x8000,
    MOVING_PLATFORM = 0x10000,
    SPRITE = 0x20000,
};


struct cmp_draw_editor_gui
{
    //nonSerialized members
    update_function * draw;
};


struct cmp_draw_game_gui
{
    //nonSerialized members
    update_function * draw;
};


struct cmp_parent
{
    //nonSerialized members
    int parentID;
    ivec2 offset;
};


struct cmp_physics
{
    //serialized members
    ivec2 vel;
    ivec2 accel;
};


struct cmp_rides_platforms
{
    //nonSerialized members
    int platformID;
    ivec2 lastVel;
    ivec2 size;
    int16_t pushedH;
    int16_t pushedV;
};


struct cmp_save_and_load
{
    //nonSerialized members
    update_function * in_out;
};


struct cmp_string_storage
{
    //nonSerialized members
    char * string;
};


struct cmp_transform
{
    //serialized members
    ivec2 pos;
    int oldY;
    float rot;
    vec2 scale;
};


struct cmp_update
{
    //nonSerialized members
    update_function * update;
};


struct cmp_waypoints
{
    //serialized members
    int count;
    ivec2 points[3];
};


struct cmp_anim
{
    //serialized members
    char * name;
    bool playing;
    bool loop;
    float speedFactor;
    float frameTime;
    int currentFrame;
};


struct cmp_custom_flags
{
    //serialized members
    uint32_t bits;
};


struct cmp_collider
{
    //nonSerialized members
    uint16_t mask;
    uint16_t category;
    ivec2 ur;
    ivec2 bl;
    int16_t collisions[4];
};


struct cmp_special_draw
{
    //nonSerialized members
    int depth;
    update_function * draw;
};


struct cmp_dialog
{
    //nonSerialized members
    void * data;
};


struct cmp_counters
{
    //serialized members
    uint32_t counters;
    uint32_t counters2;
};


struct cmp_moving_platform
{
    //serialized members
    ivec2 bl;
    ivec2 ur;
};


struct cmp_sprite
{
    //serialized members
    int depth;
    char * name;
    uint32_t color;
};

struct game_object
{
	bool inUse;
	object_type type;
	cmp_metadata metadata;
    cmp_draw_editor_gui draw_editor_gui;
    cmp_draw_game_gui draw_game_gui;
    cmp_parent parent;
    cmp_physics physics;
    cmp_rides_platforms rides_platforms;
    cmp_save_and_load save_and_load;
    cmp_string_storage string_storage;
    cmp_transform transform;
    cmp_update update;
    cmp_waypoints waypoints;
    cmp_anim anim;
    cmp_custom_flags custom_flags;
    cmp_collider collider;
    cmp_special_draw special_draw;
    cmp_dialog dialog;
    cmp_counters counters;
    cmp_moving_platform moving_platform;
    cmp_sprite sprite;
};

global_variable game_object GameObjects[MAX_GAME_OBJECTS];
global_variable int NumGameObjects;

int AddObject(object_type type)
{
	int index = NumGameObjects;
	for (int i = 0; i < NumGameObjects; i++)
	{
		if (!GameObjects[i].inUse)
		{
			index = i;
			break;
		}
	}
	if (index == NumGameObjects)
		NumGameObjects++;

    assert(NumGameObjects < MAX_GAME_OBJECTS);

	game_object *go = &GameObjects[index];
	go->inUse = true;
    
    go->type = type;
    go->metadata = {};
    go->draw_editor_gui = {};
    go->draw_game_gui = {};
    go->parent = {};
    go->physics = {};
    go->rides_platforms = {};
    go->save_and_load = {};
    go->string_storage = {};
    go->transform = {};
    go->update = {};
    go->waypoints = {};
    go->anim = {};
    go->custom_flags = {};
    go->collider = {};
    go->special_draw = {};
    go->dialog = {};
    go->counters = {};
    go->moving_platform = {};
    go->sprite = {};
    
    return index;
}

void _RemoveObject(int id)
{
	GameObjects[id].inUse = false;
	if (id == NumGameObjects-1)
		NumGameObjects--;
}


#define GO(component) (&GameObjects[goId].component)
#define OTH(id, component) (&GameObjects[id].component)

void InitObject(int goId)
{
    auto meta = GO(metadata);
    if (meta->cmpInUse & DRAW_EDITOR_GUI)
    {
        auto member = GO(draw_editor_gui);
        member->draw = NULL;
    }
    if (meta->cmpInUse & DRAW_GAME_GUI)
    {
        auto member = GO(draw_game_gui);
        member->draw = NULL;
    }
    if (meta->cmpInUse & PARENT)
    {
        auto member = GO(parent);
        member->parentID = -1;
        member->offset = vec2(0,0);
    }
    if (meta->cmpInUse & PHYSICS)
    {
        auto member = GO(physics);
    }
    if (meta->cmpInUse & RIDES_PLATFORMS)
    {
        auto member = GO(rides_platforms);
        member->platformID = -1;
    }
    if (meta->cmpInUse & SAVE_AND_LOAD)
    {
        auto member = GO(save_and_load);
        member->in_out = NULL;
    }
    if (meta->cmpInUse & STRING_STORAGE)
    {
        auto member = GO(string_storage);
        member->string = NULL;
    }
    if (meta->cmpInUse & TRANSFORM)
    {
        auto member = GO(transform);
        member->oldY = 0;
        member->rot = 0.0f;
        member->scale = vec2(1.000000f,1.000000f);
    }
    if (meta->cmpInUse & UPDATE)
    {
        auto member = GO(update);
        member->update = NULL;
    }
    if (meta->cmpInUse & WAYPOINTS)
    {
        auto member = GO(waypoints);
        member->count = 0;
    }
    if (meta->cmpInUse & ANIM)
    {
        auto member = GO(anim);
        member->name = NULL;
        member->playing = true;
        member->loop = true;
        member->speedFactor = 1.000000f;
        member->frameTime = 0.000000f;
        member->currentFrame = 0;
    }
    if (meta->cmpInUse & CUSTOM_FLAGS)
    {
        auto member = GO(custom_flags);
        member->bits = 0;
    }
    if (meta->cmpInUse & COLLIDER)
    {
        auto member = GO(collider);
        member->mask = 65535;
        member->category = 1;
    }
    if (meta->cmpInUse & SPECIAL_DRAW)
    {
        auto member = GO(special_draw);
        member->depth = 0;
        member->draw = NULL;
    }
    if (meta->cmpInUse & DIALOG)
    {
        auto member = GO(dialog);
        member->data = NULL;
    }
    if (meta->cmpInUse & COUNTERS)
    {
        auto member = GO(counters);
        member->counters = 0;
        member->counters2 = 0;
    }
    if (meta->cmpInUse & MOVING_PLATFORM)
    {
        auto member = GO(moving_platform);
    }
    if (meta->cmpInUse & SPRITE)
    {
        auto member = GO(sprite);
        member->depth = 0;
        member->name = NULL;
        member->color = 4294967295;
    }
}
object_type GetObjectType(char *type)
{
    if (strcmp(type, "OBJ_NONE") == 0)
        return OBJ_NONE;
    if (strcmp(type, "OBJ_DIALOGUE_BOX") == 0)
        return OBJ_DIALOGUE_BOX;
    if (strcmp(type, "OBJ_PLAYER") == 0)
        return OBJ_PLAYER;
    if (strcmp(type, "OBJ_HEALTH_BAR") == 0)
        return OBJ_HEALTH_BAR;
    if (strcmp(type, "OBJ_KNOCKBACK") == 0)
        return OBJ_KNOCKBACK;
    if (strcmp(type, "OBJ_SWORD_SWIPE") == 0)
        return OBJ_SWORD_SWIPE;
    if (strcmp(type, "OBJ_DUST") == 0)
        return OBJ_DUST;
    if (strcmp(type, "OBJ_CRYSTAL_GENERATOR") == 0)
        return OBJ_CRYSTAL_GENERATOR;
    if (strcmp(type, "OBJ_GENERATOR_DISPLAY") == 0)
        return OBJ_GENERATOR_DISPLAY;
    if (strcmp(type, "OBJ_UPGRADE_PANEL") == 0)
        return OBJ_UPGRADE_PANEL;
    if (strcmp(type, "OBJ_CRYSTAL") == 0)
        return OBJ_CRYSTAL;
    if (strcmp(type, "OBJ_PLATFORM") == 0)
        return OBJ_PLATFORM;
    if (strcmp(type, "OBJ_MAGIC_BULLET") == 0)
        return OBJ_MAGIC_BULLET;
    if (strcmp(type, "OBJ_MAGIC_BULLET_TRAIL_DRAWER") == 0)
        return OBJ_MAGIC_BULLET_TRAIL_DRAWER;
    if (strcmp(type, "OBJ_BASIC_BULLET_DRAWER") == 0)
        return OBJ_BASIC_BULLET_DRAWER;
    if (strcmp(type, "OBJ_GAME_CAMERA") == 0)
        return OBJ_GAME_CAMERA;
    if (strcmp(type, "OBJ_ROCK") == 0)
        return OBJ_ROCK;
    if (strcmp(type, "OBJ_DOOR") == 0)
        return OBJ_DOOR;
    if (strcmp(type, "OBJ_RAIN_GENERATOR") == 0)
        return OBJ_RAIN_GENERATOR;
    if (strcmp(type, "OBJ_LEVEL_LOADER") == 0)
        return OBJ_LEVEL_LOADER;
    if (strcmp(type, "OBJ_LEVEL_SAVER") == 0)
        return OBJ_LEVEL_SAVER;
    if (strcmp(type, "OBJ_DEBUG_DISPLAY") == 0)
        return OBJ_DEBUG_DISPLAY;
    if (strcmp(type, "OBJ_TILE_DRAWER") == 0)
        return OBJ_TILE_DRAWER;
    if (strcmp(type, "OBJ_MODE_SELECTOR") == 0)
        return OBJ_MODE_SELECTOR;
    if (strcmp(type, "OBJ_TILE_PLACER") == 0)
        return OBJ_TILE_PLACER;
    if (strcmp(type, "OBJ_CAMERA_RESTRAINT_PLACER") == 0)
        return OBJ_CAMERA_RESTRAINT_PLACER;
    if (strcmp(type, "OBJ_PREFAB_SELECTOR") == 0)
        return OBJ_PREFAB_SELECTOR;
    if (strcmp(type, "OBJ_PREFAB_MOVER") == 0)
        return OBJ_PREFAB_MOVER;
    if (strcmp(type, "OBJ_PREFAB_PLACER") == 0)
        return OBJ_PREFAB_PLACER;
    if (strcmp(type, "OBJ_LEVEL_SELECTOR") == 0)
        return OBJ_LEVEL_SELECTOR;
    if (strcmp(type, "OBJ_EDIT_OBJECTS") == 0)
        return OBJ_EDIT_OBJECTS;
    if (strcmp(type, "OBJ_DEBUG_FUNCTIONS") == 0)
        return OBJ_DEBUG_FUNCTIONS;
    if (strcmp(type, "OBJ_STANDIN") == 0)
        return OBJ_STANDIN;
    if (strcmp(type, "OBJ_ENEMY_JUMPER") == 0)
        return OBJ_ENEMY_JUMPER;
    if (strcmp(type, "OBJ_ENEMY_PROJECTILE") == 0)
        return OBJ_ENEMY_PROJECTILE;
    if (strcmp(type, "OBJ_ENEMY_SWORD_SWIPE") == 0)
        return OBJ_ENEMY_SWORD_SWIPE;
    if (strcmp(type, "OBJ_ENEMY_SWOOPER") == 0)
        return OBJ_ENEMY_SWOOPER;
    return OBJ_NONE;
}


component_type GetComponentType(char *type)
{
    if (strcmp(type, "DRAW_EDITOR_GUI") == 0)
        return DRAW_EDITOR_GUI;
    if (strcmp(type, "DRAW_GAME_GUI") == 0)
        return DRAW_GAME_GUI;
    if (strcmp(type, "PARENT") == 0)
        return PARENT;
    if (strcmp(type, "PHYSICS") == 0)
        return PHYSICS;
    if (strcmp(type, "RIDES_PLATFORMS") == 0)
        return RIDES_PLATFORMS;
    if (strcmp(type, "SAVE_AND_LOAD") == 0)
        return SAVE_AND_LOAD;
    if (strcmp(type, "STRING_STORAGE") == 0)
        return STRING_STORAGE;
    if (strcmp(type, "TRANSFORM") == 0)
        return TRANSFORM;
    if (strcmp(type, "UPDATE") == 0)
        return UPDATE;
    if (strcmp(type, "WAYPOINTS") == 0)
        return WAYPOINTS;
    if (strcmp(type, "ANIM") == 0)
        return ANIM;
    if (strcmp(type, "CUSTOM_FLAGS") == 0)
        return CUSTOM_FLAGS;
    if (strcmp(type, "COLLIDER") == 0)
        return COLLIDER;
    if (strcmp(type, "SPECIAL_DRAW") == 0)
        return SPECIAL_DRAW;
    if (strcmp(type, "DIALOG") == 0)
        return DIALOG;
    if (strcmp(type, "COUNTERS") == 0)
        return COUNTERS;
    if (strcmp(type, "MOVING_PLATFORM") == 0)
        return MOVING_PLATFORM;
    if (strcmp(type, "SPRITE") == 0)
        return SPRITE;
    return METADATA;
}


#include "serialization_helper.h"


void AnimationUpdate(int goId);

int DeserializeObject(json_value *val)
{
	int goId = AddObject(GetObjectType(val->hash->GetByKey("type")->string));
	auto meta = GO(metadata);
	json_value *components = val->hash->GetByKey("components");
	json_value *item = components->array->first;
	while (item)
	{
		meta->cmpInUse |= GetComponentType(item->string);
		item = item->next;
	}
    InitObject(goId);

	json_value *data = val->hash->GetByKey("data");
	json_hash_element *el = data->hash->first;

	while (el)
	{
		if (strcmp(el->key, "draw_editor_gui") == 0)
		{
			auto c = GO(draw_editor_gui);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "draw") == 0)
                {
                    c->draw = GetFunction(member->value->string);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "draw_game_gui") == 0)
		{
			auto c = GO(draw_game_gui);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "draw") == 0)
                {
                    c->draw = GetFunction(member->value->string);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "parent") == 0)
		{
			auto c = GO(parent);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "parentID") == 0)
                {
                    DeserializeInt(member->value, &c->parentID);
                }
				else if (strcmp(member->key, "offset") == 0)
                {
                    DeserializeIvec2(member->value, &c->offset);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "physics") == 0)
		{
			auto c = GO(physics);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "vel") == 0)
                {
                    DeserializeIvec2(member->value, &c->vel);
                }
				else if (strcmp(member->key, "accel") == 0)
                {
                    DeserializeIvec2(member->value, &c->accel);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "rides_platforms") == 0)
		{
			auto c = GO(rides_platforms);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "platformID") == 0)
                {
                    DeserializeInt(member->value, &c->platformID);
                }
				else if (strcmp(member->key, "lastVel") == 0)
                {
                    DeserializeIvec2(member->value, &c->lastVel);
                }
				else if (strcmp(member->key, "size") == 0)
                {
                    DeserializeIvec2(member->value, &c->size);
                }
				else if (strcmp(member->key, "pushedH") == 0)
                {
                }
				else if (strcmp(member->key, "pushedV") == 0)
                {
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "save_and_load") == 0)
		{
			auto c = GO(save_and_load);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "in_out") == 0)
                {
                    c->in_out = GetFunction(member->value->string);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "string_storage") == 0)
		{
			auto c = GO(string_storage);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "string") == 0)
                {
                    DeserializeCharStar(member->value, &c->string);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "transform") == 0)
		{
			auto c = GO(transform);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "pos") == 0)
                {
                    DeserializeIvec2(member->value, &c->pos);
                }
				else if (strcmp(member->key, "oldY") == 0)
                {
                    DeserializeInt(member->value, &c->oldY);
                }
				else if (strcmp(member->key, "rot") == 0)
                {
                    DeserializeFloat(member->value, &c->rot);
                }
				else if (strcmp(member->key, "scale") == 0)
                {
                    DeserializeVec2(member->value, &c->scale);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "update") == 0)
		{
			auto c = GO(update);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "update") == 0)
                {
                    c->update = GetFunction(member->value->string);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "waypoints") == 0)
		{
			auto c = GO(waypoints);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "count") == 0)
                {
                    DeserializeInt(member->value, &c->count);
                }
				else if (strcmp(member->key, "points") == 0)
                {
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "anim") == 0)
		{
			auto c = GO(anim);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "name") == 0)
                {
                    DeserializeCharStar(member->value, &c->name);
                }
				else if (strcmp(member->key, "playing") == 0)
                {
                    DeserializeBool(member->value, &c->playing);
                }
				else if (strcmp(member->key, "loop") == 0)
                {
                    DeserializeBool(member->value, &c->loop);
                }
				else if (strcmp(member->key, "speedFactor") == 0)
                {
                    DeserializeFloat(member->value, &c->speedFactor);
                }
				else if (strcmp(member->key, "frameTime") == 0)
                {
                    DeserializeFloat(member->value, &c->frameTime);
                }
				else if (strcmp(member->key, "currentFrame") == 0)
                {
                    DeserializeInt(member->value, &c->currentFrame);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "custom_flags") == 0)
		{
			auto c = GO(custom_flags);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "bits") == 0)
                {
                    DeserializeUint32(member->value, &c->bits);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "collider") == 0)
		{
			auto c = GO(collider);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "mask") == 0)
                {
                    DeserializeUint16(member->value, &c->mask);
                }
				else if (strcmp(member->key, "category") == 0)
                {
                    DeserializeUint16(member->value, &c->category);
                }
				else if (strcmp(member->key, "ur") == 0)
                {
                    DeserializeIvec2(member->value, &c->ur);
                }
				else if (strcmp(member->key, "bl") == 0)
                {
                    DeserializeIvec2(member->value, &c->bl);
                }
				else if (strcmp(member->key, "collisions") == 0)
                {
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "special_draw") == 0)
		{
			auto c = GO(special_draw);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "depth") == 0)
                {
                    DeserializeInt(member->value, &c->depth);
                }
				else if (strcmp(member->key, "draw") == 0)
                {
                    c->draw = GetFunction(member->value->string);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "dialog") == 0)
		{
			auto c = GO(dialog);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "data") == 0)
                {
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "counters") == 0)
		{
			auto c = GO(counters);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "counters") == 0)
                {
                    DeserializeUint32(member->value, &c->counters);
                }
				else if (strcmp(member->key, "counters2") == 0)
                {
                    DeserializeUint32(member->value, &c->counters2);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "moving_platform") == 0)
		{
			auto c = GO(moving_platform);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "bl") == 0)
                {
                    DeserializeIvec2(member->value, &c->bl);
                }
				else if (strcmp(member->key, "ur") == 0)
                {
                    DeserializeIvec2(member->value, &c->ur);
                }
				member = member->next;
			}
		}
		else if (strcmp(el->key, "sprite") == 0)
		{
			auto c = GO(sprite);
			json_hash_element *member = el->value->hash->first;
			while (member)
			{
				if (strcmp(member->key, "depth") == 0)
                {
                    DeserializeInt(member->value, &c->depth);
                }
				else if (strcmp(member->key, "name") == 0)
                {
                    DeserializeCharStar(member->value, &c->name);
                }
				else if (strcmp(member->key, "color") == 0)
                {
                    DeserializeUint32(member->value, &c->color);
                }
				member = member->next;
			}
		}
        else { assert(false); } //didn't find member
        el = el->next;
	}

    meta->flags |= GAME_OBJECT;
    if ((meta->cmpInUse & ANIM) && (meta->cmpInUse & SPRITE))
        AnimationUpdate(goId);

	return goId;
}
