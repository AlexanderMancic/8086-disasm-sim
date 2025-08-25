#pragma once

#include "types.h"
#include "instruction.h"

[[nodiscard]]
u16 DoArithmetic(u16 dst, u16 src, u16 *flags_register, Instruction *inst);
