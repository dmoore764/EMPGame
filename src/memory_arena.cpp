void InitArena(memory_arena *arena, size_t size)
{
	arena->size = size;
	arena->memory = calloc(size, 1);
	arena->current = arena->memory;
}

transient_arena GetTransientArena(memory_arena *arena)
{
	transient_arena result;
	result.arena = arena;
	result.current = arena->current;
	return result;
}

void FreeTransientArena(transient_arena ta)
{
	ta.arena->current = ta.current;
}

void *_PushData(memory_arena *arena, size_t size)
{
	assert((char *)arena->current + size < (char *)arena->memory + arena->size);
	void *result = arena->current;
	arena->current = (void *)((char *)arena->current + size);
	return result;
}
