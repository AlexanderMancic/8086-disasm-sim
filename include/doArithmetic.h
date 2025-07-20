#pragma once

#include "types.h"

[[nodiscard]]
u16 doArithmetic(u16 dst, u16 src, u16 *flagsRegister, u8 arithOpcode, u8 w);
