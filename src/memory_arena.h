#pragma once

struct memory_arena
{
	void *memory;
	void *current;
	size_t size;
};

struct transient_arena
{
	memory_arena *arena;
	void *current;
};

//Allocate a large amount of memory for in-game usage
void InitArena(memory_arena *arena, size_t size);

//Basically used to create allocate a large amount of memory for a short period of time.
//The memory allocated between this call and FreeTransientArena will all be erased, so 
//stuff that needs to be permanent should be made outside of these calls.
transient_arena GetTransientArena(memory_arena *arena);
void FreeTransientArena(transient_arena ta);


void *_PushData(memory_arena *arena, size_t size);

//These are memory allocators
//PUSH_ITEM(&Arena, int) is equivalent to (int *)malloc(sizeof(int)), except it goes on our memory arena
#define PUSH_ITEM(Arena, Type) (Type *)_PushData(Arena, sizeof(Type))
//PUSH_ARRAY(&Arena, int, 10) is equivalent to (int *)malloc(10*sizeof(int)), except it goes on our memory arena
#define PUSH_ARRAY(Arena, Type, Count) (Type *)_PushData(Arena, sizeof(Type)*Count)
