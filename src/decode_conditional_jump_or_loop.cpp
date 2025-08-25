#include "decode_conditional_jump_or_loop.h"

void DecodeConditionalJumpOrLoop(Instruction *inst, u8 *ram, u16 *ip)
{
			*ip += 1;

			inst->ip_inc8 = (i8)ram[*ip];
			*ip += 1;

			inst->jump_ip = *ip + inst->ip_inc8;
}
