#include "getAddress.h"
#include "types.h"

i32 getAddress(u16 *registers[8], u16 displacement, u8 mod, u8 rm) {

	// {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

	if (mod == 00 && rm == 0b110) {
		return displacement;
	}

	switch (rm) {
		
		case 0:
			return *registers[3] + *registers[6] + displacement;

		case 1:
			return *registers[3] + *registers[7] + displacement;

		case 2:
			return *registers[5] + *registers[6] + displacement;

		case 3:
			return *registers[5] + *registers[7] + displacement;

		case 4:
			return *registers[6] + displacement;

		case 5:
			return *registers[7] + displacement;

		case 6:
			return *registers[5] + displacement;

		case 7:
			return *registers[3] + displacement;
	}

	return -1;
}
