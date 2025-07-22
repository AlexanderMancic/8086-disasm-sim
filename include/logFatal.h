#ifndef LOGFATAL_H
#define LOGFATAL_H

#include <stdio.h>

#include "arena.h"

void logFatal(Arena *arena, int inputFD, FILE *outputFile, const char *errMessage);

#endif
