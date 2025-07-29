#pragma once

#include "arena.h"
#include <cstdio>

[[noreturn]]
void Cleanup(int exit_code, int input_fd, FILE *output_file, Arena *arena);
