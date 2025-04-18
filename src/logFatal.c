#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logFatal.h"

void logFatal(int inputFD, int outputFD, char *errMessage) {
	fprintf(stderr, "%s\n", errMessage);
	if (close(inputFD) == -1) {
		perror("Error closing input file");
		if (close(outputFD) == -1) {
			perror("Error closing input file");
			exit(EXIT_FAILURE);
		}
	}
	if (close(outputFD) == -1) {
		perror("Error closing input file");
	}
	exit(EXIT_FAILURE);
}
