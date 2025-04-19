#pragma once
#include "types.h"

[[nodiscard]]
u8 getRMstring(u8 mod, u8 rm, u8 w, char *rmString, int inputFD, u32 *ip);
