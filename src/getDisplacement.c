#include "getDisplacement.h"
#include "types.h"

i32 getDisplacement(u8 *offsetRam, u8 mod, u8 rm) {

	switch (mod) {
		case 0: {
			if (rm == 0b110) {

				return (offsetRam[0] | (offsetRam[1] << 8));
			}

			return 0;
		}
		case 1: {

			return (i8)offsetRam[0];
		}
		case 2: {

			return (offsetRam[0] | (offsetRam[1] << 8));
		}
		case 3: {
			return 0;
		}
	}

	return -1;
}
