#include <stdbool.h>

#include "do_arithmetic.h"
#include "flags_reg_mask.h"
#include "register.h"

u16 DoArithmetic(u16 dst, u16 src, u16 *flags_register, Instruction *inst)
{
	u8 dst_lo_nibble = dst & 0b1111;
	u8 src_lo_nibble = src & 0b1111;
	u8 msb;
	u8 lsb;
	u8 set_lsb_bits = 0;
	u16 result;
	bool store_result = true;

	switch (inst->arith_opcode)
	{
		// add
		case 0:
		{
			if (inst->w)
			{
				if ((dst + src) > 65535)
				{
					*flags_register |= CF;
				}
				else
				{
					*flags_register &= (u16)~CF;
				}

				if
				(
					((((i16)dst) + ((i16)src)) > 32767) ||
					((((i16)dst) + ((i16)src)) < -32768)
				)
				{
					*flags_register |= OF;
				}
				else
				{
					*flags_register &= (u16)~OF;
				}
			}
			else
			{
				if ((dst + src) > 255)
				{
					*flags_register |= CF;
				}
				else
				{
					*flags_register &= (u16)~CF;
				}

				if
				(
					((((i8)dst) + ((i8)src)) > 127) ||
					((((i8)dst) + ((i8)src)) < -128)
				)
				{
					*flags_register |= OF;
				}
				else
				{
					*flags_register &= (u16)~OF;
				}
			}

			if ((dst_lo_nibble + src_lo_nibble) > 15)
			{
				*flags_register |= AF;
			}
			else
			{
				*flags_register &= (u16)~AF;
			}

			result = dst + src;
		} break;
		// sub
		case 5:
		{
			if (inst->w)
			{
				if
				(
					((((i16)dst) - ((i16)src)) > 32767) ||
					((((i16)dst) - ((i16)src)) < -32768)
				)
				{
					*flags_register |= OF;
				}
				else
				{
					*flags_register &= (u16)~OF;
				}
			}
			else
			{
				if
				(
					((((i8)dst) - ((i8)src)) > 127) ||
					((((i8)dst) - ((i8)src)) < -128)
				)
				{
					*flags_register |= OF;
				}
				else
				{
					*flags_register &= (u16)~OF;
				}
			}
			
			if (src_lo_nibble > dst_lo_nibble)
			{
				*flags_register |= AF;
			}
			else
			{
				*flags_register &= (u16)~AF;
			}

			if (src > dst)
			{
				*flags_register |= CF;
			}
			else
			{
				*flags_register &= (u16)~CF;
			}

			result = dst - src;
		} break;
		// cmp
		case 7:
		{
			if (inst->w)
			{
				if
				(
					((((i16)dst) - ((i16)src)) > 32767) ||
					((((i16)dst) - ((i16)src)) < -32768)
				)
				{
					*flags_register |= OF;
				}
				else
				{
					*flags_register &= (u16)~OF;
				}
			}
			else
			{
				if
				(
					((((i8)dst) - ((i8)src)) > 127) ||
					((((i8)dst) - ((i8)src)) < -128)
				)
				{
					*flags_register |= OF;
				}
				else
				{
					*flags_register &= (u16)~OF;
				}
			}

			if (src_lo_nibble > dst_lo_nibble)
			{
				*flags_register |= AF;
			}
			else
			{
				*flags_register &= (u16)~AF;
			}

			if (src > dst)
			{
				*flags_register |= CF;
			}
			else
			{
				*flags_register &= (u16)~CF;
			}

			result = dst - src;
			store_result = false;
		} break;
	}

	if (inst->w)
	{
		Register split_result = { .word = result };
		lsb = split_result.byte.lo;
		msb = split_result.byte.hi;
	}
	else
	{
		lsb = (u8)result;
		msb = lsb;
	}

	while (lsb)
	{
		set_lsb_bits += lsb & 1;
		lsb >>= 1;
	}

	if (set_lsb_bits % 2 == 0)
	{
		*flags_register |= PF;
	}
	else
	{
		*flags_register &= (u16)~PF;
	}

	if (result == 0)
	{
		*flags_register |= ZF;
	}
	else
	{
		*flags_register &= (u16)~ZF;
	}

	if ((msb >> 7) == 1)
	{
		*flags_register |= SF;
	}
	else
	{
		*flags_register &= (u16)~SF;
	}

	if (store_result)
	{
		return result;
	}

	return dst;
}
