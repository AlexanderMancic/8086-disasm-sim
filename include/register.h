#pragma once

#include "types.h"

union Register
{
    u16 word;

    struct
    {
        u8 lo;
        u8 hi;
    } byte;
};
