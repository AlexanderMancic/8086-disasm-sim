#include "cleanup.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

void Cleanup(int exit_code, int input_fd, FILE *output_file, Arena *arena)
{
	int final_exit_code = exit_code;
	if (close(input_fd) == -1) {
		perror("Error closing input file");
		final_exit_code = EXIT_FAILURE;
	}
	if (fclose(output_file) == EOF) {
		perror("Error closing output file");
		final_exit_code = EXIT_FAILURE;
	}
	FreeArena(arena);
	exit(final_exit_code);
}
