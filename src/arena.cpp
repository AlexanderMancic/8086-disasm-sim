#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "arena.h"

bool InitializeArena(Arena *arena, size_t capacity)
{
	arena->base = (char *)malloc(capacity);
	if (arena->base == NULL) {
		perror("Error allocating Arena memory");
		return false;
	}
	memset(arena->base, 0, capacity);
	arena->size = capacity;
	arena->used = 0;
	return true;
}

void FreeArena(Arena *arena)
{
	free(arena->base);
	arena->base = NULL;
	arena->size = 0;
	arena->used = 0;
}

u8 *AllocateU8(Arena *arena, size_t allocation_size)
{
	if (arena->used + allocation_size > arena->size) return NULL;
	arena->used += allocation_size;
	return (u8 *)arena->base + arena->used;
}

char *AllocateChar(Arena *arena, size_t allocation_size)
{
	if (arena->used + allocation_size > arena->size) return NULL;
	arena->used += allocation_size;
	return arena->base + arena->used;
}
