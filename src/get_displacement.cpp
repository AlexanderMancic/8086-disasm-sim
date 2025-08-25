#include "get_displacement.h"
#include "types.h"

i32 GetDisplacement(u8 *offset_ram, u8 mod, u8 rm)
{
	switch (mod)
	{
		case 0:
		{
			if (rm == 0b110) {

				return (offset_ram[0] | (offset_ram[1] << 8));
			}

			return 0;
		}
		case 1:
		{
			return (i8)offset_ram[0];
		}
		case 2:
		{
			return (offset_ram[0] | (offset_ram[1] << 8));
		}
		case 3:
		{
			return 0;
		}
	}

	return -1;
}
