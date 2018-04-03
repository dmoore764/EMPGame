#pragma once

struct cmp_transform
{
	vec2 position;
	float rotation;
	float scale;
	float radius;
};

struct cmp_physics
{
	vec2 velocity;
	vec2 linear_acceleration;
	float angle;
	float angular_acceleration;
};

struct cmp_test
{
	uint32_t sound_id;
	int number_of_fades_remaining;
	bool fade_out_complete;
};

struct Component {
	enum Type {
		TRANSFORM,
		PHYSICS,
		TEST,
		TYPE_COUNT // Must be at the end of the list
	};

	size_t gameObjectId;
	Type type;
	union {
		cmp_transform transform;
		cmp_physics physics;
		cmp_test test;
	};
};
