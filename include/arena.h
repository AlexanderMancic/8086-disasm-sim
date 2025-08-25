#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "types.h"

struct Arena
{
    char *base;
    size_t size;
    size_t used;
};

bool InitializeArena(Arena *arena, size_t capacity);
void FreeArena(Arena *arena);
u8 *AllocateU8(Arena *arena, size_t allocation_size);
char *AllocateChar(Arena *arena, size_t allocation_size);
