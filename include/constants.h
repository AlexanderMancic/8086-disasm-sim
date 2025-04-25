#pragma once

#include "types.h"

constexpr u8 maxRmStringSize = 16;
constexpr u8 rmBufferSize = maxRmStringSize + 1;
constexpr u8 minRmStringSize = 2;

constexpr u8 regStringSize = 2;
constexpr u8 regBufferSize = regStringSize + 1;

constexpr u8 maxRmRegMovInstStringSizeWithOutNewline = maxRmStringSize + regStringSize + 6;
constexpr u8 minRmRegMovInstStringSizeWithOutNewline = minRmStringSize + regStringSize + 6;
constexpr u8 maxRmRegMovInstStringSizeWithNewline = minRmRegMovInstStringSizeWithOutNewline + 1;
constexpr u8 minRmRegMovInstStringSizeWithNewline = maxRmRegMovInstStringSizeWithNewline + 1;
constexpr u8 rmRegMovInstBufferSize = maxRmRegMovInstStringSizeWithOutNewline + 1;

constexpr u8 simSuffixStringSize = 22;
constexpr u8 simSuffixBufferSize = simSuffixStringSize + 1;
