#include <stddef.h>

#include "get_opcode.h"

bool GetOpcode(Instruction *inst, u8 byte)
{
	// NOTE: sorted to allow implementation of binary search later
	constexpr Opcode opcodes[] =
	{
		Opcode::ADD_RM_REG,
	    Opcode::ADD_IMM_ACC,
	    Opcode::SUB_RM_REG,
	    Opcode::SUB_IMM_ACC,
	    Opcode::CMP_RM_REG,
	    Opcode::CMP_IMM_ACC,
	    Opcode::JO,
	    Opcode::JNO,
	    Opcode::JB,
	    Opcode::JAE,
	    Opcode::JZ,
	    Opcode::JNZ,
	    Opcode::JBE,
	    Opcode::JA,
	    Opcode::JS,
	    Opcode::JNS,
	    Opcode::JP,
	    Opcode::JNP,
	    Opcode::JL,
	    Opcode::JGE,
	    Opcode::JLE,
	    Opcode::JG,
	    Opcode::ADD_SUB_CMP_IMM_RM,
	    Opcode::MOV_RM_REG,
	    Opcode::MOV_SR_RM,
	    Opcode::MOV_RM_SR,
	    Opcode::MOV_MEM_ACC,
	    Opcode::MOV_ACC_MEM,
	    Opcode::MOV_IMM_REG,
	    Opcode::MOV_IMM_RM,
	    Opcode::LOOPNZ,
	    Opcode::LOOPZ,
	    Opcode::LOOP,
	    Opcode::JCXZ,
	};
	
	u8 mask = 0xFF;

	for (u8 i = 0; i < 8; i++)
	{
		for (u8 j = 0; j < sizeof(opcodes); j++)
		{
			if ((byte & mask) == (u8)opcodes[j])
			{
				inst->opcode = opcodes[j];
				return true;
			}
		}

		mask = (u8)(mask << 1);
	}
	
	return false;
}
