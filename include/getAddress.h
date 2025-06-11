#pragma once

#include "types.h"

[[nodiscard]]
i32 getAddress(u16 *registers[8], u16 displacement, u8 mod, u8 rm);
