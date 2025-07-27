#pragma once

#include "register.h"
#include "types.h"

typedef struct {

    i32 jumpIP;

    Register data;

    u16 addr;

    u8 opcode;
    u8 d;
    u8 w;
    u8 reg;
    u8 mod;
    u8 rm;
    u8 sr;
    u8 arithOpcode;
    u8 s;

    i8 ip_inc8;

} Instruction;

void ResetInstruction(Instruction *inst);
