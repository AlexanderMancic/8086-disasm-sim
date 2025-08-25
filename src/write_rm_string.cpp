#include <string.h>
#include <stdio.h>

#include "write_rm_string.h"
#include "get_reg_string.h"
#include "constants.h"
#include "types.h"

i8 WriteRmString(char *rm_string, u8 *offset_ram, u8 mod, u8 rm, u8 w)
{
	constexpr const char *const effectiveAddressBase[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

	switch (mod)
	{
		case 0:
		{
			if (rm == 0b110)
			{
				u16 directAddress = (u16)(offset_ram[0] | (offset_ram[1] << 8));
				snprintf(rm_string, MAX_OPERAND, "[%hu]", directAddress);
				return 2;
			}

			snprintf(rm_string, MAX_OPERAND, "[%s]", effectiveAddressBase[rm]);
			return 0;
		}
		case 1:
		{
			i8 disp = (i8)offset_ram[0];
			snprintf(rm_string, MAX_OPERAND, "[%s %+hhd]", effectiveAddressBase[rm], disp);
			return 1;
		}
		case 2:
		{
			i16 disp = (i16)(offset_ram[0] | (offset_ram[1] << 8));
			snprintf(rm_string, MAX_OPERAND, "[%s %+hd]", effectiveAddressBase[rm], disp);
			return 2;
		}
		case 3:
		{
			strncpy(rm_string, GetRegString(rm, w), 3);
			return 0;
		}
	}

	return -1;
}
