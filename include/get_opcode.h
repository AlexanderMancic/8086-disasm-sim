#pragma once

#include "instruction.h"
#include "types.h"

[[nodiscard]]
bool GetOpcode(Instruction *inst, u8 byte);

enum class Opcode : u8
{
    ADD_RM_REG = 0,
    ADD_IMM_ACC = 4,
    SUB_RM_REG = 40,
    SUB_IMM_ACC = 44,
    CMP_RM_REG = 56,
    CMP_IMM_ACC = 60,
    JO = 112,
    JNO = 113,
    JB = 114,
    JAE = 115,
    JZ = 116,
    JNZ = 117,
    JBE = 118,
    JA = 119,
    JS = 120,
    JNS = 121,
    JP = 122,
    JNP = 123,
    JL = 124,
    JGE = 125,
    JLE = 126,
    JG = 127,
    ADD_SUB_CMP_IMM_RM = 128,
    MOV_RM_REG = 136,
    MOV_SR_RM = 140,
    MOV_RM_SR = 142,
    MOV_MEM_ACC = 160,
    MOV_ACC_MEM = 162,
    MOV_IMM_REG = 176,
    MOV_IMM_RM = 198,
    LOOPNZ = 224,
    LOOPZ = 225,
    LOOP = 226,
    JCXZ = 227,
};