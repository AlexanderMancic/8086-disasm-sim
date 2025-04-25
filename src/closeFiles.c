#include <stdio.h>
#include <unistd.h>

#include "closeFiles.h"

void closeFiles(int fd, FILE *outfile1, FILE *outfile2) {
	
	if ((close(fd)) == -1) {
		perror("Error: Failed to close file");
	}
	
	if (outfile1 != NULL) {
		if ((fclose(outfile1)) == EOF) {
			perror("Error: Failed to close file");
		}
	}

	if (outfile2 != NULL) {
		if ((fclose(outfile2)) == EOF) {
			perror("Error: Failed to close file");
		}
	}
}
