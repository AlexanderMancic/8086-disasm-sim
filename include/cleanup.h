#pragma once

#include <stdio.h>

#include "arena.h"


[[noreturn]]
void Cleanup(int exit_code, int input_fd, FILE *output_file, Arena *arena);
