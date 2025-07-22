#include <stdlib.h>
#include <unistd.h>

#include "arena.h"

bool initializeArena(Arena *arena, size_t capacity) {
	arena->base = malloc(capacity);
	if (arena->base == NULL) return false;
	arena->size = capacity;
	arena->used = 0;
	return true;
}

void freeArena(Arena *arena) {
	free(arena->base);
	arena->base = NULL;
	arena->size = 0;
	arena->used = 0;
}

u8 *allocateU8(Arena *arena, size_t allocationSize) {
	if (arena->used + allocationSize > arena->size) return NULL;
	return (u8 *)arena->base + arena->used;
}

char *allocateChar(Arena *arena, size_t allocationSize) {
	if (arena->used + allocationSize > arena->size) return NULL;
	return arena->base + arena->used;
}
