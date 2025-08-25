#pragma once

#include "register.h"
#include "types.h"

enum class Opcode : u8;

struct Instruction
{
    i32 jump_ip;

    Register data;

    u16 addr;

    Opcode opcode;
    u8 d;
    u8 w;
    u8 reg;
    u8 mod;
    u8 rm;
    u8 sr;
    u8 arith_opcode;
    u8 s;

    i8 ip_inc8;

};

void ResetInstruction(Instruction *inst);
