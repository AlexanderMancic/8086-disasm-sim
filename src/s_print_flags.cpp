#include <stdlib.h>

#include "s_print_flags.h"
#include "types.h"

u8 SPrintFlags(char *flags_string, u16 flags_register)
{
	u8 index = 0;

	if ((flags_register & 1) == 1)
	{
		flags_string[index++] = 'C';
	}
	if (((flags_register >> 2) & 1) == 1)
	{
		flags_string[index++] = 'P';
	}
	if (((flags_register >> 4) & 1) == 1)
	{
		flags_string[index++] = 'A';
	}
	if (((flags_register >> 6) & 1) == 1)
	{
		flags_string[index++] = 'Z';
	}
	if (((flags_register >> 7) & 1) == 1)
	{
		flags_string[index++] = 'S';
	}
	if (((flags_register >> 8) & 1) == 1)
	{
		flags_string[index++] = 'T';
	}
	if (((flags_register >> 9) & 1) == 1)
	{
		flags_string[index++] = 'I';
	}
	if (((flags_register >> 10) & 1) == 1)
	{
		flags_string[index++] = 'D';
	}
	if (((flags_register >> 11) & 1) == 1)
	{
		flags_string[index] = 'O';
	}

	return EXIT_SUCCESS;
}
