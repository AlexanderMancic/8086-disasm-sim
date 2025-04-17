#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>

#include "types.h"
#include "writeOutput.h"
#include "logFatal.h"

#define MAX_OPERAND 17

static const char *regNamesW0[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
static const char *regNamesW1[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
static inline const char *getRegName(u8 reg, u8 w) {
	return w ? regNamesW1[reg] : regNamesW0[reg];
}

static const char *arithMnemonics[8] = {
	"add",
	NULL,
	NULL,
	NULL,
	NULL,
	"sub",
	NULL,
	"cmp"
};

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

		u8 instBuffer[6] = {0};
		ssize_t bytesRead = read(inputFD, &instBuffer[0], 2);

		if (bytesRead == 0) {
			break;
		} else if (bytesRead != 2) {
			logFatal(inputFD, outputFD, "Error reading the input file");
		}

		// mov r/m to/from reg
		if (instBuffer[0] >> 2 == 0b100010) {

			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};
			char regString[MAX_OPERAND] = {0};
			char rmString[MAX_OPERAND] = {0};
			u8 reg = (instBuffer[1] >> 3) & 0b111;
			u8 w = instBuffer[0] & 1;
			u8 d = (instBuffer[0] >> 1) & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;

			strncpy(regString, getRegName(reg, w), 3);

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
							bytesRead = read(inputFD, &instBuffer[2], 2);
							if (bytesRead != 2) {
								logFatal(inputFD, outputFD, "Error reading the input file");
							}
							i16 disp = instBuffer[2] | (instBuffer[3] << 8);
							snprintf(rmString, sizeof(rmString), "[%+d]", disp);
							break;
						case 7:
							strncpy(rmString, "[bx]", 5);
							break;
					}
					break;
				case 1: {
					bytesRead = read(inputFD, &instBuffer[2], 1);
					if (bytesRead != 1) {
						logFatal(inputFD, outputFD, "Error reading the input file");
					}
					i8 disp = instBuffer[2];

					switch (rm) {
						case 0:
							snprintf(rmString, sizeof(rmString), "[bx + si %+d]", disp);
							break;
						case 1:
							snprintf(rmString, sizeof(rmString), "[bx + di %+d]", disp);
							break;
						case 2:
							snprintf(rmString, sizeof(rmString), "[bp + si %+d]", disp);
							break;
						case 3:
							snprintf(rmString, sizeof(rmString), "[bp + di %+d]", disp);
							break;
						case 4:
							snprintf(rmString, sizeof(rmString), "[si %+d]", disp);
							break;
						case 5:
							snprintf(rmString, sizeof(rmString), "[di %+d]", disp);
							break;
						case 6:
							snprintf(rmString, sizeof(rmString), "[bp %+d]", disp);
							break;
						case 7:
							snprintf(rmString, sizeof(rmString), "[bx %+d]", disp);
							break;
					}
					break;
				}
				case 2: {
					bytesRead = read(inputFD, &instBuffer[2], 2);
					if (bytesRead != 2) {
						logFatal(inputFD, outputFD, "Error reading the input file");
					}
					i16 disp = instBuffer[2] | (instBuffer[3] << 8);

					switch (rm) {
						case 0:
							snprintf(rmString, sizeof(rmString), "[bx + si %+d]", disp);
							break;
						case 1:
							snprintf(rmString, sizeof(rmString), "[bx + di %+d]", disp);
							break;
						case 2:
							snprintf(rmString, sizeof(rmString), "[bp + si %+d]", disp);
							break;
						case 3:
							snprintf(rmString, sizeof(rmString), "[bp + di %+d]", disp);
							break;
						case 4:
							snprintf(rmString, sizeof(rmString), "[si %+d]", disp);
							break;
						case 5:
							snprintf(rmString, sizeof(rmString), "[di %+d]", disp);
							break;
						case 6:
							snprintf(rmString, sizeof(rmString), "[bp %+d]", disp);
							break;
						case 7:
							snprintf(rmString, sizeof(rmString), "[bx %+d]", disp);
							break;
					}
					break;
				}
				case 3:
					strncpy(rmString, getRegName(rm, w), 3);
					break;
			}

			if (d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
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
		// mov imm to r/m
		else if (instBuffer[0] >> 1 == 0b1100011) {

			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};
			u8 w = instBuffer[0] & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;
			u8 readIndex = 2;
			u16 imm = 0;

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
							readIndex += 2;
							bytesRead = read(inputFD, &instBuffer[2], 2);
							if (bytesRead != 2) {
								logFatal(inputFD, outputFD, "Error reading the input file");
							}
							i16 disp = instBuffer[2] | (instBuffer[3] << 8);
							snprintf(rmString, sizeof(rmString), "[%+d]", disp);
							break;
						case 7:
							strncpy(rmString, "[bx]", 5);
							break;
					}
					break;
				case 1: {
					readIndex += 1;
					bytesRead = read(inputFD, &instBuffer[2], 1);
					if (bytesRead != 1) {
						logFatal(inputFD, outputFD, "Error reading the input file");
					}
					i8 disp = instBuffer[2];

					switch (rm) {
						case 0:
							snprintf(rmString, sizeof(rmString), "[bx + si %+d]", disp);
							break;
						case 1:
							snprintf(rmString, sizeof(rmString), "[bx + di %+d]", disp);
							break;
						case 2:
							snprintf(rmString, sizeof(rmString), "[bp + si %+d]", disp);
							break;
						case 3:
							snprintf(rmString, sizeof(rmString), "[bp + di %+d]", disp);
							break;
						case 4:
							snprintf(rmString, sizeof(rmString), "[si %+d]", disp);
							break;
						case 5:
							snprintf(rmString, sizeof(rmString), "[di %+d]", disp);
							break;
						case 6:
							snprintf(rmString, sizeof(rmString), "[bp %+d]", disp);
							break;
						case 7:
							snprintf(rmString, sizeof(rmString), "[bx %+d]", disp);
							break;
					}
					break;
				}
				case 2: {
					readIndex += 2;
					bytesRead = read(inputFD, &instBuffer[2], 2);
					if (bytesRead != 2) {
						logFatal(inputFD, outputFD, "Error reading the input file");
					}
					i16 disp = instBuffer[2] | (instBuffer[3] << 8);

					switch (rm) {
						case 0:
							snprintf(rmString, sizeof(rmString), "[bx + si %+d]", disp);
							break;
						case 1:
							snprintf(rmString, sizeof(rmString), "[bx + di %+d]", disp);
							break;
						case 2:
							snprintf(rmString, sizeof(rmString), "[bp + si %+d]", disp);
							break;
						case 3:
							snprintf(rmString, sizeof(rmString), "[bp + di %+d]", disp);
							break;
						case 4:
							snprintf(rmString, sizeof(rmString), "[si %+d]", disp);
							break;
						case 5:
							snprintf(rmString, sizeof(rmString), "[di %+d]", disp);
							break;
						case 6:
							snprintf(rmString, sizeof(rmString), "[bp %+d]", disp);
							break;
						case 7:
							snprintf(rmString, sizeof(rmString), "[bx %+d]", disp);
							break;
					}
					break;
				}
				case 3:
					strncpy(rmString, getRegName(rm, w), 3);
					break;
			}

			if (w) {
				bytesRead = read(inputFD, &instBuffer[readIndex], 2);
				if (bytesRead != 2) {
					logFatal(inputFD, outputFD, "Error reading the input file");
				}
				imm = instBuffer[readIndex] | (instBuffer[readIndex + 1] << 8);
			} else {
				bytesRead = read(inputFD, &instBuffer[readIndex], 1);
				if (bytesRead != 1) {
					logFatal(inputFD, outputFD, "Error reading the input file");
				}
				imm = instBuffer[readIndex];
			}

			if (mod == 0 && rm == 0b110) {
				snprintf(immString, sizeof(immString), "%u", imm);
			} else {
				snprintf(immString, sizeof(immString), "%s %u", sizeSpecifier[w], imm);
			}

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
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
			} else {
				imm = instBuffer[1];
			}

			strncpy(regString, getRegName(reg, w), 3);

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
		// mov memory to accumulator
		else if (instBuffer[0] >> 1 == 0b1010000) {

			bytesRead = read(inputFD, &instBuffer[2], 1);
			if (bytesRead != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}

			u16 addr = instBuffer[1] | (instBuffer[2] << 8);
			char addrString[8] = {0};
			snprintf(addrString, sizeof(addrString), "[%u]", addr);

			if (writeOutput(inputFD, outputFD, "mov ax, ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, addrString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// mov accumulator to memory
		else if (instBuffer[0] >> 1 == 0b1010001) {

			bytesRead = read(inputFD, &instBuffer[2], 1);
			if (bytesRead != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}

			u16 addr = instBuffer[1] | (instBuffer[2] << 8);
			char addrString[8] = {0};
			snprintf(addrString, sizeof(addrString), "[%u]", addr);

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, addrString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ax\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// add/sub/cmp with/and reg to either
		else if ((instBuffer[0] & 0b11000100) == 0) {

			u8 arithOpcode = instBuffer[0] >> 3;
			u8 d = (instBuffer[0] >> 1) & 1;
			u8 w = instBuffer[0] & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 reg = (instBuffer[1] >> 3) & 0b111;
			u8 rm = instBuffer[1] & 0b111;
			char regString[3] = {0};
			char rmString[MAX_OPERAND] = {0};
			char arithMnemonic[4] = {0};
			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};

			strncpy(arithMnemonic, arithMnemonics[arithOpcode], 3);
			strncpy(regString, getRegName(reg, w), 3);

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
							bytesRead = read(inputFD, &instBuffer[2], 2);
							if (bytesRead != 2) {
								logFatal(inputFD, outputFD, "Error reading the input file");
							}
							i16 disp = instBuffer[2] | (instBuffer[3] << 8);
							snprintf(rmString, sizeof(rmString), "[%+d]", disp);
							break;
						case 7:
							strncpy(rmString, "[bx]", 5);
							break;
					}
					break;
				case 1: {
					bytesRead = read(inputFD, &instBuffer[2], 1);
					if (bytesRead != 1) {
						logFatal(inputFD, outputFD, "Error reading the input file");
					}
					i8 disp = instBuffer[2];

					switch (rm) {
						case 0:
							snprintf(rmString, sizeof(rmString), "[bx + si %+d]", disp);
							break;
						case 1:
							snprintf(rmString, sizeof(rmString), "[bx + di %+d]", disp);
							break;
						case 2:
							snprintf(rmString, sizeof(rmString), "[bp + si %+d]", disp);
							break;
						case 3:
							snprintf(rmString, sizeof(rmString), "[bp + di %+d]", disp);
							break;
						case 4:
							snprintf(rmString, sizeof(rmString), "[si %+d]", disp);
							break;
						case 5:
							snprintf(rmString, sizeof(rmString), "[di %+d]", disp);
							break;
						case 6:
							snprintf(rmString, sizeof(rmString), "[bp %+d]", disp);
							break;
						case 7:
							snprintf(rmString, sizeof(rmString), "[bx %+d]", disp);
							break;
					}
					break;
				}
				case 2: {
					bytesRead = read(inputFD, &instBuffer[2], 2);
					if (bytesRead != 2) {
						logFatal(inputFD, outputFD, "Error reading the input file");
					}
					i16 disp = instBuffer[2] | (instBuffer[3] << 8);

					switch (rm) {
						case 0:
							snprintf(rmString, sizeof(rmString), "[bx + si %+d]", disp);
							break;
						case 1:
							snprintf(rmString, sizeof(rmString), "[bx + di %+d]", disp);
							break;
						case 2:
							snprintf(rmString, sizeof(rmString), "[bp + si %+d]", disp);
							break;
						case 3:
							snprintf(rmString, sizeof(rmString), "[bp + di %+d]", disp);
							break;
						case 4:
							snprintf(rmString, sizeof(rmString), "[si %+d]", disp);
							break;
						case 5:
							snprintf(rmString, sizeof(rmString), "[di %+d]", disp);
							break;
						case 6:
							snprintf(rmString, sizeof(rmString), "[bp %+d]", disp);
							break;
						case 7:
							snprintf(rmString, sizeof(rmString), "[bx %+d]", disp);
							break;
					}
					break;
				}
				case 3:
					strncpy(rmString, getRegName(rm, w), 3);
					break;
			}


			if (d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
			}

			if (writeOutput(inputFD, outputFD, arithMnemonic) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, " ") == EXIT_FAILURE) {
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
