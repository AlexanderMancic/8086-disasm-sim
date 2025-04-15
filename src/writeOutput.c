#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "writeOutput.h"

int writeOutput(int inputFD, int outputFD, char *text) {
	if (write(outputFD, text, strlen(text)) == -1) {
		perror("Error writing to outfile");
		if (close(inputFD) == -1) {
			perror("Error closing input file");
			if (close(outputFD) == -1) {
				perror("Error closing input file");
			}
		}
		if (close(outputFD) == -1) {
			perror("Error closing input file");
		}
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

