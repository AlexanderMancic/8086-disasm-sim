#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "writeRMstring.h"
#include "getRegString.h"
#include "constants.h"

i8 writeRMstring(char *rmString, u8 *offsetRam, u8 mod, u8 rm, u8 w) {

	const char *const effectiveAddressBase[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

	switch (mod) {
		case 0: {
			if (rm == 0b110) {

				u16 directAddress = (u16)offsetRam[1] | (u16)(offsetRam[2] << 8);
				snprintf(rmString, MAX_OPERAND, "[%hu]", directAddress);
				return 2;
			}

			snprintf(rmString, MAX_OPERAND, "[%s]", effectiveAddressBase[rm]);
			return 0;
		}
		case 1: {

			i8 disp = (i8)offsetRam[1];
			snprintf(rmString, MAX_OPERAND, "[%s %+hhd]", effectiveAddressBase[rm], disp);
			return 1;
		}
		case 2: {

			i16 disp = (u16)offsetRam[1] | (u16)(offsetRam[2] << 8);
			snprintf(rmString, MAX_OPERAND, "[%s %+hd]", effectiveAddressBase[rm], disp);
			return 2;
		}
		case 3: {
			strncpy(rmString, getRegString(rm, w), 3);
			return 0;
		}
	}

	return -1;
}
