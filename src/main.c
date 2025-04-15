#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "types.h"
#include "writeOutput.h"

int main(int argc, char **argv) {

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <infile> <outfile>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int inputFD = open(argv[1], O_RDONLY);
	if (inputFD == -1) {
		perror("Error opening input file");
		return EXIT_FAILURE;
	}

	int outputFD = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (outputFD == -1) {
		perror("Error opening output file");
		if (close(inputFD) == -1) {
			perror("Error closing input file");
		}
		return EXIT_FAILURE;
	}

	if (writeOutput(inputFD, outputFD, "bits 16\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	while (true) {

		u8 instBuffer[2] = {0};
		ssize_t bytesRead = read(inputFD, &instBuffer[0], 2);

		if (bytesRead == 0) {
			break;
		} else if (bytesRead != 2) {
			fprintf(stderr, "Error reading the input file");
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

		if (instBuffer[0] >> 2 == 0b100010) {

			char dst[3] = {0};
			char src[3] = {0};
			char regName[3] = {0};
			char rmName[3] = {0};
			u8 reg = (instBuffer[1] >> 3) & 0b111;
			u8 w = instBuffer[0] & 1;
			u8 d = (instBuffer[0] >> 1) & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;

			if (w) {
				switch (reg) {
					case 0:
						strncpy(regName, "ax", 3);
						break;
					case 1:
						strncpy(regName, "cx", 3);
						break;
					case 2:
						strncpy(regName, "dx", 3);
						break;
					case 3:
						strncpy(regName, "bx", 3);
						break;
					case 4:
						strncpy(regName, "sp", 3);
						break;
					case 5:
						strncpy(regName, "bp", 3);
						break;
					case 6:
						strncpy(regName, "si", 3);
						break;
					case 7:
						strncpy(regName, "di", 3);
						break;
				}
			} else {
				switch (reg) {
					case 0:
						strncpy(regName, "al", 3);
						break;
					case 1:
						strncpy(regName, "cl", 3);
						break;
					case 2:
						strncpy(regName, "dl", 3);
						break;
					case 3:
						strncpy(regName, "bl", 3);
						break;
					case 4:
						strncpy(regName, "ah", 3);
						break;
					case 5:
						strncpy(regName, "ch", 3);
						break;
					case 6:
						strncpy(regName, "dh", 3);
						break;
					case 7:
						strncpy(regName, "bh", 3);
						break;
				}
			}

			if (mod != 0b11) {
				fprintf(stderr, "Error: this version only support register to register movs");
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
			if (w) {
				switch (rm) {
					case 0:
						strncpy(rmName, "ax", 3);
						break;
					case 1:
						strncpy(rmName, "cx", 3);
						break;
					case 2:
						strncpy(rmName, "dx", 3);
						break;
					case 3:
						strncpy(rmName, "bx", 3);
						break;
					case 4:
						strncpy(rmName, "sp", 3);
						break;
					case 5:
						strncpy(rmName, "bp", 3);
						break;
					case 6:
						strncpy(rmName, "si", 3);
						break;
					case 7:
						strncpy(rmName, "di", 3);
						break;
				}
			} else {
				switch (rm) {
					case 0:
						strncpy(rmName, "al", 3);
						break;
					case 1:
						strncpy(rmName, "cl", 3);
						break;
					case 2:
						strncpy(rmName, "dl", 3);
						break;
					case 3:
						strncpy(rmName, "bl", 3);
						break;
					case 4:
						strncpy(rmName, "ah", 3);
						break;
					case 5:
						strncpy(rmName, "ch", 3);
						break;
					case 6:
						strncpy(rmName, "dh", 3);
						break;
					case 7:
						strncpy(rmName, "bh", 3);
						break;
				}
			}

			if (d) {
				strncpy(dst, regName, 2);
				strncpy(src, rmName, 2);
			} else {
				strncpy(dst, rmName, 2);
				strncpy(src, regName, 2);
			}

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, dst) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, src) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		else {
			fprintf(stderr, "Unknown opcode");
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
	}

	if (close(inputFD) == -1) {
		perror("Error closing input file");
		if (close(outputFD) == -1) {
			perror("Error closing input file");
		}
		return EXIT_FAILURE;
	}
	if (close(outputFD) == -1) {
		perror("Error closing input file");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
