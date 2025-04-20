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
#include "register.h"

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

	Register ax = {"ax", 0};
	Register bx = {"bx", 0};
	Register cx = {"cx", 0};
	Register dx = {"dx", 0};
	Register sp = {"sp", 0};
	Register bp = {"bp", 0};
	Register si = {"si", 0};
	Register di = {"di", 0};
	Register genRegisters[8] = {ax, cx, dx, bx, sp, bp, si, di};

	Register es = {"es", 0};
	Register cs = {"cs", 0};
	Register ss = {"ss", 0};
	Register ds = {"ds", 0};
	Register segRegisters[4] = {es, cs, ss, ds};

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

			if (mod == 3) {
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				if (d) {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", genRegisters[reg].value);
					genRegisters[reg].value = genRegisters[rm].value;
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", genRegisters[reg].value);
				} else {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", genRegisters[rm].value);
					genRegisters[rm].value = genRegisters[reg].value;
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", genRegisters[rm].value);
				}

				if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, dst) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
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

			char beforeRegValue[6] = {0};
			char afterRegValue[6] = {0};

			snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", genRegisters[reg].value);
			genRegisters[reg].value = (u16)imm;
			snprintf(afterRegValue, sizeof(afterRegValue), "%hu", genRegisters[reg].value);

			if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, regString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ": ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, beforeRegValue) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, " -> ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, afterRegValue) == EXIT_FAILURE) {
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
			u16 addr = (u16)instBuffer[1] | (u16)(instBuffer[2] << 8);
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
			u16 addr = (u16)instBuffer[1] | (u16)(instBuffer[2] << 8);
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
		// mov rm to sr
		else if (instBuffer[0] == 0b10001110) {

			if (read(inputFD, &instBuffer[1], 1) != 1) {
				logFatal(inputFD, outputFD, "Error reading the input file");
			}
			ip += 1;

			u8 mod = instBuffer[1] >> 6;
			u8 sr = (instBuffer[1] >> 3) & 0b11;
			u8 rm = instBuffer[1] & 0b111;
			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			if (getRMstring(mod, rm, 1, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, outputFD, "Error getting the rm string");
			}

			strncpy(srString, segRegisters[sr].string, 3);

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, srString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

			if (mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", segRegisters[sr].value);
				segRegisters[sr].value = genRegisters[rm].value;
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", segRegisters[sr].value);

				if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, srString) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
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
		// je / jz
		else if (instBuffer[0] == 0b01110100) {

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

			if (writeOutput(inputFD, outputFD, "jz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jl / jnge
		else if (instBuffer[0] == 0b01111100) {

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

			if (writeOutput(inputFD, outputFD, "jl IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jle / jng
		else if (instBuffer[0] == 0b01111110) {

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

			if (writeOutput(inputFD, outputFD, "jle IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jb / jnae
		else if (instBuffer[0] == 0b01110010) {

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

			if (writeOutput(inputFD, outputFD, "jb IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jbe / jna
		else if (instBuffer[0] == 0b01110110) {

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

			if (writeOutput(inputFD, outputFD, "jbe IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jp / jpe
		else if (instBuffer[0] == 0b01111010) {

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

			if (writeOutput(inputFD, outputFD, "jp IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jo
		else if (instBuffer[0] == 0b01110000) {

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

			if (writeOutput(inputFD, outputFD, "jo IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// js
		else if (instBuffer[0] == 0b01111000) {

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

			if (writeOutput(inputFD, outputFD, "js IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jne / jnz
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
		// jnl / jge
		else if (instBuffer[0] == 0b01111101) {

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

			if (writeOutput(inputFD, outputFD, "jge IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnle / jg
		else if (instBuffer[0] == 0b01111111) {

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

			if (writeOutput(inputFD, outputFD, "jg IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnb / jae
		else if (instBuffer[0] == 0b01110011) {

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

			if (writeOutput(inputFD, outputFD, "jae IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnbe / ja
		else if (instBuffer[0] == 0b01110111) {

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

			if (writeOutput(inputFD, outputFD, "ja IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnp / jpo
		else if (instBuffer[0] == 0b01111011) {

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

			if (writeOutput(inputFD, outputFD, "jnp IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jno
		else if (instBuffer[0] == 0b01110001) {

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

			if (writeOutput(inputFD, outputFD, "jno IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jns
		else if (instBuffer[0] == 0b01111001) {

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

			if (writeOutput(inputFD, outputFD, "jns IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// loop
		else if (instBuffer[0] == 0b11100010) {

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

			if (writeOutput(inputFD, outputFD, "loop IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// loopz / loope
		else if (instBuffer[0] == 0b11100001) {

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

			if (writeOutput(inputFD, outputFD, "loopz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// loopnz / loopne
		else if (instBuffer[0] == 0b11100000) {

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

			if (writeOutput(inputFD, outputFD, "loopnz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jcxz
		else if (instBuffer[0] == 0b11100011) {

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

			if (writeOutput(inputFD, outputFD, "jcxz IP_") == EXIT_FAILURE) {
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

	if (writeOutput(inputFD, outputFD, "\n\n; Final Registers:\n\n; General Purpose Registers\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	for (u8 i = 0; i < 8; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", genRegisters[i].value);

		if (writeOutput(inputFD, outputFD, ";\t") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, genRegisters[i].string) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, ": ") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, regValueString) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, outputFD, "; Segment Registers:\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	for (u8 i = 0; i < 4; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", segRegisters[i].value);

		if (writeOutput(inputFD, outputFD, ";\t") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, segRegisters[i].string) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, ": ") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, regValueString) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
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
