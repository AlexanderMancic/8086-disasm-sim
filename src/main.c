#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "types.h"
#include "writeOutput.h"
#include "logFatal.h"

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

		u8 instBuffer[3] = {0};
		ssize_t bytesRead = read(inputFD, &instBuffer[0], 2);

		if (bytesRead == 0) {
			break;
		} else if (bytesRead != 2) {
			logFatal(inputFD, outputFD, "Error reading the input file");
		}

		// mov r/m to/from reg
		if (instBuffer[0] >> 2 == 0b100010) {

			char dst[3] = {0};
			char src[10] = {0};
			char regString[3] = {0};
			char rmString[10] = {0};
			u8 reg = (instBuffer[1] >> 3) & 0b111;
			u8 w = instBuffer[0] & 1;
			u8 d = (instBuffer[0] >> 1) & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;

			if (w) {
				switch (reg) {
					case 0:
						strncpy(regString, "ax", 3);
						break;
					case 1:
						strncpy(regString, "cx", 3);
						break;
					case 2:
						strncpy(regString, "dx", 3);
						break;
					case 3:
						strncpy(regString, "bx", 3);
						break;
					case 4:
						strncpy(regString, "sp", 3);
						break;
					case 5:
						strncpy(regString, "bp", 3);
						break;
					case 6:
						strncpy(regString, "si", 3);
						break;
					case 7:
						strncpy(regString, "di", 3);
						break;
				}
			} else {
				switch (reg) {
					case 0:
						strncpy(regString, "al", 3);
						break;
					case 1:
						strncpy(regString, "cl", 3);
						break;
					case 2:
						strncpy(regString, "dl", 3);
						break;
					case 3:
						strncpy(regString, "bl", 3);
						break;
					case 4:
						strncpy(regString, "ah", 3);
						break;
					case 5:
						strncpy(regString, "ch", 3);
						break;
					case 6:
						strncpy(regString, "dh", 3);
						break;
					case 7:
						strncpy(regString, "bh", 3);
						break;
				}
			}

			switch (mod) {
				case 0:
					switch (rm) {
						case 0:
							strncpy(rmString, "[bx + si]", 10);
							break;
						case 1:
							strncpy(rmString, "[bx + di]", 10);
							break;
						case 2:
							strncpy(rmString, "[bp + si]", 10);
							break;
						case 3:
							strncpy(rmString, "[bp + di]", 10);
							break;
						case 4:
							strncpy(rmString, "[si]", 5);
							break;
						case 5:
							strncpy(rmString, "[di]", 5);
							break;
						case 6:
							logFatal(
								inputFD,
								outputFD,
								"Error: this version doesn't support direct address movs"
							);
							break;
						case 7:
							strncpy(rmString, "[bx]", 5);
							break;
					}
					break;
				case 1:
				case 2:
					logFatal(
						inputFD,
						outputFD,
						"Error: this version only supports movs in register or memory (without displacement) mode"
					);
					break;
				case 3:
					if (w) {
						switch (rm) {
							case 0:
								strncpy(rmString, "ax", 3);
								break;
							case 1:
								strncpy(rmString, "cx", 3);
								break;
							case 2:
								strncpy(rmString, "dx", 3);
								break;
							case 3:
								strncpy(rmString, "bx", 3);
								break;
							case 4:
								strncpy(rmString, "sp", 3);
								break;
							case 5:
								strncpy(rmString, "bp", 3);
								break;
							case 6:
								strncpy(rmString, "si", 3);
								break;
							case 7:
								strncpy(rmString, "di", 3);
								break;
						}
					} else {
						switch (rm) {
							case 0:
								strncpy(rmString, "al", 3);
								break;
							case 1:
								strncpy(rmString, "cl", 3);
								break;
							case 2:
								strncpy(rmString, "dl", 3);
								break;
							case 3:
								strncpy(rmString, "bl", 3);
								break;
							case 4:
								strncpy(rmString, "ah", 3);
								break;
							case 5:
								strncpy(rmString, "ch", 3);
								break;
							case 6:
								strncpy(rmString, "dh", 3);
								break;
							case 7:
								strncpy(rmString, "bh", 3);
								break;
						}
					}
							break;
					}

			if (d) {
				strncpy(dst, regString, 2);
				strncpy(src, rmString, 10);
			} else {
				strncpy(dst, rmString, 2);
				strncpy(src, regString, 10);
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
		// mov imm to reg
		else if (instBuffer[0] >> 4 == 0b1011) {

			char regString[3] = {0};
			char immString[6] = {0};
			u8 reg = instBuffer[0] & 0b111;
			u8 w = (instBuffer[0] >> 3) & 1;
			u16 imm = 0;

			if (w) {

				bytesRead = read(inputFD, &instBuffer[2], 1);

				if (bytesRead != 1) {
					logFatal(inputFD, outputFD, "Error reading the input file");
				}

				imm = instBuffer[1] | (instBuffer[2] << 8);

				switch (reg) {
					case 0:
						strncpy(regString, "ax", 3);
						break;
					case 1:
						strncpy(regString, "cx", 3);
						break;
					case 2:
						strncpy(regString, "dx", 3);
						break;
					case 3:
						strncpy(regString, "bx", 3);
						break;
					case 4:
						strncpy(regString, "sp", 3);
						break;
					case 5:
						strncpy(regString, "bp", 3);
						break;
					case 6:
						strncpy(regString, "si", 3);
						break;
					case 7:
						strncpy(regString, "di", 3);
						break;
				}
			} else {

				imm = instBuffer[1];

				switch (reg) {
					case 0:
						strncpy(regString, "al", 3);
						break;
					case 1:
						strncpy(regString, "cl", 3);
						break;
					case 2:
						strncpy(regString, "dl", 3);
						break;
					case 3:
						strncpy(regString, "bl", 3);
						break;
					case 4:
						strncpy(regString, "ah", 3);
						break;
					case 5:
						strncpy(regString, "ch", 3);
						break;
					case 6:
						strncpy(regString, "dh", 3);
						break;
					case 7:
						strncpy(regString, "bh", 3);
						break;
				}
			}

			snprintf(immString, sizeof(immString), "%u", imm);

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, regString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, immString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		else {
			logFatal(inputFD, outputFD, "Error: Unknown opcode");
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
