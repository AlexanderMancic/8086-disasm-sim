#include <stddef.h>

#include "getOpcode.h"

bool GetOpcode(Instruction *inst, u8 byte) {

	// NOTE: sorted to allow implementation of binary search later
	const u8 opcodes[] = {
		0,
		2,
		10,
		11,
		14,
		22,
		30,
		32,
		34,
		80,
		81,
		99,
		112,
		113,
		114,
		115,
		116,
		117,
		118,
		119,
		120,
		121,
		122,
		123,
		124,
		125,
		127,
		126,
		140,
		142,
		224,
		225,
		226,
		227,
	};

	const u8 opcodeBitLengths[] = {
		8,
		7,
		6,
		4,
	};

	for (size_t i = 0; i < sizeof(opcodeBitLengths); i++) {
		
		u8 comparisonValue = byte >> (8 - opcodeBitLengths[i]);
		for (size_t j = 0; j < sizeof(opcodes); j++) {

			if (comparisonValue == opcodes[j]) {
				inst->opcode = opcodes[j];
				return true;
			}
		}
	}
	
	return false;
}
