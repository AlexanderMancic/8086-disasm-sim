#pragma once

#include "types.h"

// On error, -1 is returned.
// On success, the number to be added
// to the IP register is returned.
[[nodiscard]]
i8 WriteRmString(char *rm_string, u8 *offset_ram, u8 mod, u8 rm, u8 w);
