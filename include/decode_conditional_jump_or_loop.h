#pragma once

#include "instruction.h"
#include "types.h"

void Decode_Conditional_Jump_Or_Loop(Instruction *inst, u8 *ram, u16 *ip);
