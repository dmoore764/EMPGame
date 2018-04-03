#pragma once

enum ColType {
	ColType_NOTHING			= 0,
	ColType_THING			= 0x1,
	ColType_ROCK			= 0x2,
	ColType_SHIP			= 0x4,
	ColType_BULLET			= 0x8,
	ColType_SPAWN_LOCATION  = 0x10,
	ColType_PLAYER			= 0x20,
	ColType_ALL				= 0x3FFFFFFF,
};

using namespace glm;

struct CollisionTransform
{
	glm::vec2 offset;
	glm::vec2 pos;
	glm::vec2 scale;
	float rotation;
};

struct collision_polygon
{
	glm::vec2 center;
	int numVerts;
	glm::vec2 vertices[16];
};

struct collision_circle
{
	glm::vec2 center;
	float radius;
};


// Interface for other collision objects to derive from so that collision can be done generically.
struct collision_aabb {
	bool initialized;
	vec2 min;
	vec2 max;
};

struct CollisionObject {
	enum Type {
		AABB,
		POINT,
		CIRCLE,
		POLYGON,
	};

	Type type;

	union {
		collision_aabb aabb;
		glm::vec2 point;
		collision_polygon poly;
		collision_circle circle;
	};
};

#define MAX_COLLISION_GROUP_SIZE 5

struct CollisionGroup {
	uint32_t categoryBits;
	uint32_t maskBits;

	CollisionObject objects[MAX_COLLISION_GROUP_SIZE];
	CollisionObject transformed[MAX_COLLISION_GROUP_SIZE];
	size_t count;
};

inline float Inner(glm::vec2 a, glm::vec2 b);
inline float LengthSq(glm::vec2 a);
inline float Length(glm::vec2 a);
inline glm::vec2 Perp(glm::vec2 a);

CollisionObject ColGetTransformedShape(CollisionObject *s, CollisionTransform *tx);
bool ColShapesIntersect(CollisionObject *s1, CollisionObject *s2, glm::vec2 *separationVector = NULL);

collision_aabb GetAabbForObject(CollisionObject *shape);
void AabbAddPoint(collision_aabb* aabb, const vec2& point);
void AabbGrowToContain(collision_aabb* aabb, collision_aabb* other);
void PolygonAddVertex(collision_polygon *poly, glm::vec2 p);
