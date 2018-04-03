enum object_type
{
    OBJ_NONE,
    OBJ_DIALOGUE_BOX,
    OBJ_PLAYER,
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
};


enum component_type {
    METADATA,
    ANIM = 0x1,
    CUSTOM_FLAGS = 0x2,
    DRAW_EDITOR_GUI = 0x4,
    DRAW_GAME_GUI = 0x8,
    PHYSICS = 0x10,
    RIDES_PLATFORMS = 0x20,
    TRANSFORM = 0x40,
    UPDATE = 0x80,
    COLLIDER = 0x100,
    SPECIAL_DRAW = 0x200,
    DIALOG = 0x400,
    COUNTERS = 0x800,
    MOVING_PLATFORM = 0x1000,
    SPRITE = 0x2000,
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



struct game_components
{
    uint32_t idIndex[MAX_GAME_OBJECTS];
    object_type type[MAX_GAME_OBJECTS];
    cmp_metadata metadata[MAX_GAME_OBJECTS];
    cmp_anim anim[MAX_GAME_OBJECTS];
    cmp_custom_flags custom_flags[MAX_GAME_OBJECTS];
    cmp_draw_editor_gui draw_editor_gui[MAX_GAME_OBJECTS];
    cmp_draw_game_gui draw_game_gui[MAX_GAME_OBJECTS];
    cmp_physics physics[MAX_GAME_OBJECTS];
    cmp_rides_platforms rides_platforms[MAX_GAME_OBJECTS];
    cmp_transform transform[MAX_GAME_OBJECTS];
    cmp_update update[MAX_GAME_OBJECTS];
    cmp_collider collider[MAX_GAME_OBJECTS];
    cmp_special_draw special_draw[MAX_GAME_OBJECTS];
    cmp_dialog dialog[MAX_GAME_OBJECTS];
    cmp_counters counters[MAX_GAME_OBJECTS];
    cmp_moving_platform moving_platform[MAX_GAME_OBJECTS];
    cmp_sprite sprite[MAX_GAME_OBJECTS];
};
global_variable game_components GameComponents;
global_variable int NumGameObjects;


//the game object is a index into the arrays of components
struct GameObjectId
{
    bool inUse;
    uint32_t index;
};
global_variable GameObjectId GameObjectIDs[MAX_GAME_OBJECTS];

//stores the top most used id (could be free spots below this number though)
global_variable int idCount;

//add an object by finding the first free slot, the returned integer
//would never change for the life of the object (but the index into the
//gameObjects will)
int AddObject(object_type type)
{
    int result = -1;
    for (int i = 0; i < idCount; i++)
    {
        if (!GameObjectIDs[i].inUse)
        {
            result = i;
            break;
        }
    }
    
    //The list of game objects up to idCount is full, so add at the end
    if (result == -1)
        result = idCount++;
    
    assert(result < MAX_GAME_OBJECTS);
    
    GameObjectIDs[result].inUse = true;
    GameObjectIDs[result].index = NumGameObjects++;
    
    GameComponents.type[NumGameObjects-1] = type;
    GameComponents.idIndex[NumGameObjects-1] = result;
    GameComponents.metadata[NumGameObjects-1] = {};
    GameComponents.anim[NumGameObjects-1] = {};
    GameComponents.custom_flags[NumGameObjects-1] = {};
    GameComponents.draw_editor_gui[NumGameObjects-1] = {};
    GameComponents.draw_game_gui[NumGameObjects-1] = {};
    GameComponents.physics[NumGameObjects-1] = {};
    GameComponents.rides_platforms[NumGameObjects-1] = {};
    GameComponents.transform[NumGameObjects-1] = {};
    GameComponents.update[NumGameObjects-1] = {};
    GameComponents.collider[NumGameObjects-1] = {};
    GameComponents.special_draw[NumGameObjects-1] = {};
    GameComponents.dialog[NumGameObjects-1] = {};
    GameComponents.counters[NumGameObjects-1] = {};
    GameComponents.moving_platform[NumGameObjects-1] = {};
    GameComponents.sprite[NumGameObjects-1] = {};
    
    return result;
}

void _RemoveObject(int id)
{
    if (id == idCount-1)
        idCount--;
    
    GameObjectId *goID = &GameObjectIDs[id];
    
    if (goID->index != NumGameObjects-1)
    {
        int idIndex = GameComponents.idIndex[NumGameObjects-1];
    
        //move around the objects so the list of components is compact
        GameComponents.idIndex[goID->index] = GameComponents.idIndex[NumGameObjects-1];
        GameComponents.type[goID->index] = GameComponents.type[NumGameObjects-1];
        GameComponents.metadata[goID->index] = GameComponents.metadata[NumGameObjects-1];
        GameComponents.anim[goID->index] = GameComponents.anim[NumGameObjects-1];
        GameComponents.custom_flags[goID->index] = GameComponents.custom_flags[NumGameObjects-1];
        GameComponents.draw_editor_gui[goID->index] = GameComponents.draw_editor_gui[NumGameObjects-1];
        GameComponents.draw_game_gui[goID->index] = GameComponents.draw_game_gui[NumGameObjects-1];
        GameComponents.physics[goID->index] = GameComponents.physics[NumGameObjects-1];
        GameComponents.rides_platforms[goID->index] = GameComponents.rides_platforms[NumGameObjects-1];
        GameComponents.transform[goID->index] = GameComponents.transform[NumGameObjects-1];
        GameComponents.update[goID->index] = GameComponents.update[NumGameObjects-1];
        GameComponents.collider[goID->index] = GameComponents.collider[NumGameObjects-1];
        GameComponents.special_draw[goID->index] = GameComponents.special_draw[NumGameObjects-1];
        GameComponents.dialog[goID->index] = GameComponents.dialog[NumGameObjects-1];
        GameComponents.counters[goID->index] = GameComponents.counters[NumGameObjects-1];
        GameComponents.moving_platform[goID->index] = GameComponents.moving_platform[NumGameObjects-1];
        GameComponents.sprite[goID->index] = GameComponents.sprite[NumGameObjects-1];
        
        //if we swapped, update the index into the component lists
        GameObjectIDs[idIndex].index = goID->index;
    }
    
    //the list of object components gets packed down
    NumGameObjects--;
    
    goID->inUse = false;
}


#define GO(component) (&GameComponents.component[GameObjectIDs[goId].index])
#define OTH(id, component) (&GameComponents.component[GameObjectIDs[id].index])


void InitObject(int goId)
{
    auto meta = GO(metadata);
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
    if (meta->cmpInUse & PHYSICS)
    {
        auto member = GO(physics);
    }
    if (meta->cmpInUse & RIDES_PLATFORMS)
    {
        auto member = GO(rides_platforms);
        member->platformID = -1;
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
    if (meta->cmpInUse & COLLIDER)
    {
        auto member = GO(collider);
        member->mask = 0;
        member->category = 0;
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
    return OBJ_NONE;
}


component_type GetComponentType(char *type)
{
    if (strcmp(type, "ANIM") == 0)
        return ANIM;
    if (strcmp(type, "CUSTOM_FLAGS") == 0)
        return CUSTOM_FLAGS;
    if (strcmp(type, "DRAW_EDITOR_GUI") == 0)
        return DRAW_EDITOR_GUI;
    if (strcmp(type, "DRAW_GAME_GUI") == 0)
        return DRAW_GAME_GUI;
    if (strcmp(type, "PHYSICS") == 0)
        return PHYSICS;
    if (strcmp(type, "RIDES_PLATFORMS") == 0)
        return RIDES_PLATFORMS;
    if (strcmp(type, "TRANSFORM") == 0)
        return TRANSFORM;
    if (strcmp(type, "UPDATE") == 0)
        return UPDATE;
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
		if (strcmp(el->key, "anim") == 0)
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
		else if (strcmp(el->key, "draw_editor_gui") == 0)
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
