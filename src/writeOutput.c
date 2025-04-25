#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "writeOutput.h"
#include "types.h"

int writeOutput(int outputFD, char *text) {

	if ((write(outputFD, text, strlen(text))) != (i64)(strlen(text))) {
		return -1;
	}
	return EXIT_SUCCESS;
}

