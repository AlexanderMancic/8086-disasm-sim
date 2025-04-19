#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "getRMstring.h"
#include "getRegString.h"
#include "constants.h"
#include "types.h"

u8 getRMstring(u8 mod, u8 rm, u8 w, char *rmString, int inputFD) {
	const char *const effectiveAddressBase[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};
	u8 dispBuffer[2];

	switch (mod) {
		case 0: {
			if (rm == 0b110) {
				if ((read(inputFD, &dispBuffer, 2)) != 2) {
					return EXIT_FAILURE;
				}

				u16 directAddress = dispBuffer[0] | (dispBuffer[1] << 8);
				snprintf(rmString, MAX_OPERAND, "[%hu]", directAddress);
				return EXIT_SUCCESS;
			}

			snprintf(rmString, MAX_OPERAND, "[%s]", effectiveAddressBase[rm]);
			return EXIT_SUCCESS;
		}
		case 1: {
			if ((read(inputFD, &dispBuffer, 1)) != 1) {
				return EXIT_FAILURE;
			}

			i8 disp = (i8)dispBuffer[0];
			snprintf(rmString, MAX_OPERAND, "[%s %+hhd]", effectiveAddressBase[rm], disp);
			return EXIT_SUCCESS;
		}
		case 2: {
			if ((read(inputFD, &dispBuffer, 2)) != 2) {
				return EXIT_FAILURE;
			}

			i16 disp = dispBuffer[0] | (dispBuffer[1] << 8);
			snprintf(rmString, MAX_OPERAND, "[%s %+hd]", effectiveAddressBase[rm], disp);
			return EXIT_SUCCESS;
		}
		case 3: {
			strncpy(rmString, getRegString(rm, w), 3);
			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}
