#pragma once

#include "instruction.h"
#include "types.h"

void DecodeConditionalJumpOrLoop(Instruction *inst, u8 *ram, u16 *ip);
