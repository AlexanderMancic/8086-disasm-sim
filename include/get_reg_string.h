#pragma once

#include "types.h"

static inline const char *GetRegString(u8 reg, u8 w) __attribute__((always_inline));

static inline const char *GetRegString(u8 reg, u8 w)
{
    static const char *const regNamesW0[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
    static const char *const regNamesW1[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    return w ? regNamesW1[reg] : regNamesW0[reg];
}
