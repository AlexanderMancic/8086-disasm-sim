#pragma once

#include "types.h"

// On error, -1 is returned.
// On success, the number to be added
// to the IP register is returned.
[[nodiscard]]
i8 writeRmString(char *rmString, u8 *offsetRam, u8 mod, u8 rm, u8 w);
