#pragma once

#include "types.h"
#include "instruction.h"

[[nodiscard]]
u16 doArithmetic(u16 dst, u16 src, u16 *flagsRegister, Instruction *inst);
