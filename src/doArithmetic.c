#include <stdbool.h>

#include "doArithmetic.h"
#include "flagsRegMask.h"
#include "register.h"

u16 doArithmetic(u16 dst, u16 src, u16 *flagsRegister, u8 arithOpcode, u8 w) {

	u8 dstLoNibble = dst & 0b1111;
	u8 srcLoNibble = src & 0b1111;
	u8 msb;
	u8 lsb;
	u8 setLSBbits = 0;
	u16 result;
	bool storeResult = true;

	switch (arithOpcode) {
		// add
		case 0: {

			if (w) {

				if ((dst + src) > 65535) {
					*flagsRegister |= CF;
				} else {
				*flagsRegister &= (u16)~CF;
				}

				if (
					((((i16)dst) + ((i16)src)) > 32767) ||
					((((i16)dst) + ((i16)src)) < -32768)
				) {
					*flagsRegister |= OF;
				} else {
					*flagsRegister &= (u16)~OF;
				}

			} else {
				if ((dst + src) > 255) {
					*flagsRegister |= CF;
				} else {
				*flagsRegister &= (u16)~CF;
				}

				if (
					((((i8)dst) + ((i8)src)) > 127) ||
					((((i8)dst) + ((i8)src)) < -128)
				) {
					*flagsRegister |= OF;
				} else {
					*flagsRegister &= (u16)~OF;
				}
			}

			if ((dstLoNibble + srcLoNibble) > 15) {
				*flagsRegister |= AF;
			} else {
				*flagsRegister &= (u16)~AF;
			}

			result = dst + src;

			break;
		}
		// sub
		case 5: {

			if (w) {
				if (
					((((i16)dst) - ((i16)src)) > 32767) ||
					((((i16)dst) - ((i16)src)) < -32768)
				) {
					*flagsRegister |= OF;
				} else {
					*flagsRegister &= (u16)~OF;
				}
			} else {
				if (
					((((i8)dst) - ((i8)src)) > 127) ||
					((((i8)dst) - ((i8)src)) < -128)
				) {
					*flagsRegister |= OF;
				} else {
					*flagsRegister &= (u16)~OF;
				}
			}
			
			if (srcLoNibble > dstLoNibble) {
				*flagsRegister |= AF;
			} else {
				*flagsRegister &= (u16)~AF;
			}

			if (src > dst) {
				*flagsRegister |= CF;
			} else {
				*flagsRegister &= (u16)~CF;
			}

			result = dst - src;
			break;
		}
		// cmp
		case 7: {
			if (w) {
				if (
					((((i16)dst) - ((i16)src)) > 32767) ||
					((((i16)dst) - ((i16)src)) < -32768)
				) {
					*flagsRegister |= OF;
				} else {
					*flagsRegister &= (u16)~OF;
				}
			} else {
				if (
					((((i8)dst) - ((i8)src)) > 127) ||
					((((i8)dst) - ((i8)src)) < -128)
				) {
					*flagsRegister |= OF;
				} else {
					*flagsRegister &= (u16)~OF;
				}
			}

			if (src > dst) {
				*flagsRegister |= CF;
			} else {
				*flagsRegister &= (u16)~CF;
			}

			result = dst - src;
			storeResult = false;
			break;
		}
	}

	if (w) {
		Register splitResult = { .word = result };
		lsb = splitResult.byte.lo;
		msb = splitResult.byte.hi;
	} else {
		lsb = (u8)result;
		msb = lsb;
	}

	while (lsb) {
		setLSBbits += lsb & 1;
		lsb >>= 1;
	}

	if (setLSBbits % 2 == 0) {
		*flagsRegister |= PF;
	} else {
		*flagsRegister &= (u16)~PF;
	}

	if (result == 0) {
		*flagsRegister |= ZF;
	} else {
		*flagsRegister &= (u16)~ZF;
	}

	if ((msb >> 7) == 1) {
		*flagsRegister |= SF;
	} else {
		*flagsRegister &= (u16)~SF;
	}

	if (storeResult) {
		return result;
	}
	return dst;

}
