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
#include "getRegString.h"
#include "constants.h"
#include "getRMstring.h"
#include "getImm.h"

static const char *const arithMnemonics[8] = {
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

	u32 ip = 0;

	while (true) {

		u8 instBuffer[6];
		ssize_t bytesRead = read(inputFD, &instBuffer, 1);

		if (bytesRead == 0) {
			break;
		} else if (bytesRead != 1) {
			logFatal(inputFD, outputFD, "Error reading the input file");
		}

		char label[16] = {0};
		snprintf(label, sizeof(label), "IP_%u:\n", ip);
		if (writeOutput(inputFD, outputFD, label) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}

		ip += (u32)bytesRead;

		// mov r/m to/from reg
		if (instBuffer[0] >> 2 == 0b100010) {

			if ((read(inputFD, &instBuffer[1], 1)) != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}

			ip += 1;

			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};
			char regString[MAX_OPERAND] = {0};
			char rmString[MAX_OPERAND] = {0};
			u8 reg = (instBuffer[1] >> 3) & 0b111;
			u8 w = instBuffer[0] & 1;
			u8 d = (instBuffer[0] >> 1) & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;

			strncpy(regString, getRegString(reg, w), 3);

			if (getRMstring(mod, rm, w, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, outputFD, "Error getting the rm string");
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

			if ((read(inputFD, &instBuffer[1], 1)) != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};
			u8 w = instBuffer[0] & 1;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;

			if (getRMstring(mod, rm, w, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, outputFD, "Error getting the rm string");
			}

			i32 imm = getImm(w, inputFD, &ip);
			if (imm == -1) {
				logFatal(inputFD, outputFD, "Error decoding immediate value");
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

			i32 imm = getImm(w, inputFD, &ip);
			if (imm == -1) {
				logFatal(inputFD, outputFD, "Error decoding immediate value");
			}

			strncpy(regString, getRegString(reg, w), 3);

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

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

			if ((read(inputFD, &instBuffer[1], 2)) != 2) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}
			ip += 2;

			u8 w = instBuffer[0] & 1;
			u16 addr = instBuffer[1] | (instBuffer[2] << 8);
			char addrString[8] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			snprintf(addrString, sizeof(addrString), "[%hu]", addr);

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, (char *)accumulatorString[w]) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ") == EXIT_FAILURE) {
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

			if (read(inputFD, &instBuffer[1], 2) != 2) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}
			ip += 2;

			u8 w = instBuffer[0] & 1;
			u16 addr = instBuffer[1] | (instBuffer[2] << 8);
			char addrString[8] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			snprintf(addrString, sizeof(addrString), "[%hu]", addr);

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, addrString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, (char *)accumulatorString[w]) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// add/sub/cmp with/and reg to either
		else if ((instBuffer[0] & 0b11000100) == 0) {

			if ((read(inputFD, &instBuffer[1], 1)) != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}
			ip += 1;

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
			strncpy(regString, getRegString(reg, w), 3);

			if (getRMstring(mod, rm, w, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, outputFD, "Error getting the rm string");
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
		// add/sub/cmp imm to/from/with r/m
		else if (instBuffer[0] >> 2 == 0b100000) {

			if ((read(inputFD, &instBuffer[1], 1)) != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}
			ip += 1;

			u8 s = (instBuffer[0] >> 1) & 1;
			u8 w = instBuffer[0] & 1;
			u8 sw = 1 ? w == 1 && s == 0 : 0;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;
			u8 arithOpcode = (instBuffer[1] >> 3) & 0b111;
			char arithMnemonic[4] = {0};
			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			strncpy(arithMnemonic, arithMnemonics[arithOpcode], 3);

			if (getRMstring(mod, rm, w, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, outputFD, "Error getting the rm string");
			}

			i32 imm = getImm(sw, inputFD, &ip);
			if (imm == -1) {
				logFatal(inputFD, outputFD, "Error decoding immediate value");
			}
			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			snprintf(immString, sizeof(immString), "%s %u", sizeSpecifier[w], imm);

			if (writeOutput(inputFD, outputFD, arithMnemonic) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, " ") == EXIT_FAILURE) {
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
		// add/sub/cmp imm to/from/with accumulator
		else if ((instBuffer[0] & 0b11000110) == 0b00000100) {

			u8 w = instBuffer[0] & 1;
			u8 arithOpcode = (instBuffer[0] >> 3) & 0b111;
			char immString[6] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			i32 imm = getImm(w, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, outputFD, "Error decoding immediate value");
			}

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			if (writeOutput(inputFD, outputFD, (char *)arithMnemonics[arithOpcode]) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, " ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, (char *)accumulatorString[w]) == EXIT_FAILURE) {
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
		// jnz
		else if (instBuffer[0] == 0b01110101) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, outputFD, "Error decoding immediate value");
			}

			i8 ip_inc8 = (i8)imm;
			u32 jumpIP;
			char jumpIPstring[11] = {0};

			if (ip_inc8 < 0) {
				jumpIP = (u32)((i32)ip - (i32)(-ip_inc8));
			} else {
				jumpIP = (u32)((i32)ip + (i32)ip_inc8);
			}

			snprintf(jumpIPstring, sizeof(jumpIPstring), "%u", jumpIP);

			if (writeOutput(inputFD, outputFD, "jnz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

		}
		else {
			fprintf(stderr, "Byte: 0x%x\n", instBuffer[0]);
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
