#pragma once

#include "types.h"

typedef union {
    u16 word;
    struct {
        u8 lo;
        u8 hi;
    } byte;
} Register;

