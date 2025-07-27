#include "decode_conditional_jump_or_loop.h"

void Decode_Conditional_Jump_Or_Loop(Instruction *inst, u8 *ram, u16 *ip) {

			*ip += 1;

			inst->ip_inc8 = (i8)ram[*ip];
			*ip += 1;

			inst->jumpIP = *ip + inst->ip_inc8;
}
