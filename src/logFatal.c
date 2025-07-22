#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logFatal.h"
#include "arena.h"

void logFatal(Arena *arena, int inputFD, FILE *outputFile, const char *errMessage) {
	fprintf(stderr, "%s\n", errMessage);
	if (close(inputFD) == -1) {
		perror("Error closing input file");
		if (fclose(outputFile) == EOF) {
			perror("Error closing input file");
			freeArena(arena);
			exit(EXIT_FAILURE);
		}
	}
	if (fclose(outputFile) == EOF) {
		perror("Error closing input file");
	}
	freeArena(arena);
	exit(EXIT_FAILURE);
}
