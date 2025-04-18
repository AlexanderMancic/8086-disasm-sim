#include <unistd.h>

#include "types.h"

i32 getImm(u8 w, int inputFD) {
	u8 immBuffer[2];
	u16 imm;

	if (w) {
		if ((read(inputFD, &immBuffer, 2)) != 2) {
			return -1;
		}

		imm = immBuffer[0] | (immBuffer[1] << 8);

	} else {
		if ((read(inputFD, &immBuffer, 1)) != 1) {
			return -1;
		}

		imm = immBuffer[0];
	}

	return imm;
}
