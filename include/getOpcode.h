#pragma once

#include "instruction.h"
#include "types.h"

[[nodiscard]]
bool GetOpcode(Instruction *inst, u8 byte);
