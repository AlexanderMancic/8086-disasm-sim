#pragma once

#include "types.h"

static inline const char *GetSrString(u8 sr) __attribute__((always_inline));

static inline const char *GetSrString(u8 sr)
{
    static const char *const srNames[4] = {"es", "cs", "ss", "ds"};
    return srNames[sr];
}
