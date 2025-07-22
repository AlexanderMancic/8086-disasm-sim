#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "types.h"

typedef struct {
    char *base;
    size_t size;
    size_t used;
} Arena;

bool initializeArena(Arena *arena, size_t capacity);
void freeArena(Arena *arena);
u8 *allocateU8(Arena *arena, size_t allocationSize);
char *allocateChar(Arena *arena, size_t allocationSize);
