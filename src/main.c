#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>

#include "doArithmetic.h"
#include "getDisplacement.h"
#include "getAddress.h"
#include "getOpcode.h"
#include "getSRstring.h"
#include "types.h"
#include "sPrintFlags.h"
#include "getRegString.h"
#include "constants.h"
#include "writeRmString.h"
#include "register.h"
#include "flagsRegMask.h"
#include "arena.h"
#include "instruction.h"
#include "decode_conditional_jump_or_loop.h"

#define GET_REG_VAL(w, reg) ((w) ? *registersW1[reg] : *registersW0[reg])
#define SET_REG_VAL(w, reg, val) ((w) ? (*registersW1[reg] = ((u16)val)) : (*registersW0[reg] = ((u8)val)))
#define isFlagSet(flagMask) ((flagsRegister & flagMask) != 0)

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

	bool sim = false;
	bool dump = false;
	const char *usage = "Usage: %s <infile> <outfile> [simulate [dump]]\n";
	if (argc == 5) {
		if (strncmp(argv[3], "simulate", 8) != 0) {
			fprintf(stderr, usage, argv[0]);
			return EXIT_FAILURE;
		}
		sim = true;

		if (strncmp(argv[4], "dump", 4) != 0) {
			fprintf(stderr, usage, argv[0]);
			return EXIT_FAILURE;
		}
		dump = true;
	} else if (argc == 4 ) {
		if (strncmp(argv[3], "simulate", 8) != 0) {
			fprintf(stderr, usage, argv[0]);
			return EXIT_FAILURE;
		}
		sim = true;
	} else if (argc != 3) {
		fprintf(stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}

	int inputFD = open(argv[1], O_RDONLY);
	if (inputFD == -1) {
		perror("Error opening input file");
		return EXIT_FAILURE;
	}

	FILE *outputFile = fopen(argv[2], "w");
	if (outputFile == NULL) {
		perror("Error opening output file");
		if (close(inputFD) == -1) {
			perror("Error closing input file");
		}
		return EXIT_FAILURE;
	}

	Arena arena;
	if (!initializeArena(&arena, MiB)) {
		fprintf(stderr, "Error allocating memory\n");
		goto cleanup;
	}

	u8 *ram = allocateU8(&arena, 65536);
	if (fprintf(outputFile, "bits 16\n") < 0) {
		fprintf(stderr, "Error writing to output file\n");
		goto cleanup;
	}

	u16 ip = 0;

	Register ax = { .word = 0 };
	Register bx = { .word = 0 };
	Register cx = { .word = 0 };
	Register dx = { .word = 0 };
	u16 sp = 0;
	u16 bp = 0;
	u16 si = 0;
	u16 di = 0;

	u8 *registersW0[8] = {&ax.byte.lo, &cx.byte.lo, &dx.byte.lo, &bx.byte.lo, &ax.byte.hi, &cx.byte.hi, &dx.byte.hi, &bx.byte.hi};
	u16 *registersW1[8] = {&ax.word, &cx.word, &dx.word, &bx.word, &sp, &bp, &si, &di};

	u16 es = 0;
	u16 cs = 0;
	u16 ss = 0;
	u16 ds = 0;
	u16 *segRegisters[4] = {&es, &cs, &ss, &ds};

	u16 flagsRegister = 0;

	struct stat inputStat;
	if (fstat(inputFD, &inputStat) == -1) {
		perror("Error reading stats of input file\n");
		goto cleanup;
	}

	ssize_t bytesRead = read(inputFD, ram, (size_t)inputStat.st_size);
	if (bytesRead != inputStat.st_size) {
		fprintf(stderr, "Error reading input file\n");
		goto cleanup;
	}
	if (bytesRead > 65536) {
		fprintf(stderr, "Error: Input file size must not exceed 65,536 bytes\n");
		goto cleanup;
	}

	Instruction inst;

	while (ip < bytesRead) {

		ResetInstruction(&inst);

		if (fprintf(outputFile, "IP_%u:\n", ip) < 0) {
			fprintf(stderr, "Error writing to output file\n");
			goto cleanup;
		}

		if (!GetOpcode(&inst, ram[ip])) {
			fprintf(stderr, "Error getting opcode\n");
			goto cleanup;
		}

		// mov r/m to/from reg
		if (inst.opcode == 0b100010) {

			inst.d = (ram[ip] >> 1) & 1;
			inst.w = ram[ip] & 1;
			ip += 1;

			inst.mod = ram[ip] >> 6;
			inst.reg = (ram[ip] >> 3) & 0b111;
			inst.rm = ram[ip] & 0b111;
			ip += 1;

			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};
			char regString[MAX_OPERAND] = {0};
			char rmString[MAX_OPERAND] = {0};

			strncpy(regString, getRegString(inst.reg, inst.w), 3);

			i32 disp;
			if (sim) {
				disp = getDisplacement(&ram[ip], inst.mod, inst.rm);
				if (disp == -1) {
					fprintf(stderr, "Error getting displacement value\n");
					goto cleanup;
				}
			}

			i8 ipOffset = writeRmString(rmString, &ram[ip], inst.mod, inst.rm, inst.w);
			if (ipOffset == -1) {
				fprintf(stderr, "Error writing rm string to buffer\n");
				goto cleanup;
			}
			ip += (u16)ipOffset;

			if (inst.d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
			}

			if (fprintf(outputFile, "mov %s, %s", dst, src) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && inst.mod == 3) {
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				if (inst.d) {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(inst.w, inst.reg));
					SET_REG_VAL(inst.w, inst.reg, GET_REG_VAL(inst.w, inst.rm));
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(inst.w, inst.reg));
				} else {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(inst.w, inst.rm));
					SET_REG_VAL(inst.w, inst.rm, GET_REG_VAL(inst.w, inst.reg));
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(inst.w, inst.rm));
				}

				if (fprintf(outputFile, " ; %s: %s -> %s", dst, beforeRegValue, afterRegValue) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (sim && inst.mod != 3) {
				i32 address = getAddress(registersW1, (u16)disp, inst.mod, inst.rm);
				if (address == -1) {
					fprintf(stderr, "Error calculating address\n");
					goto cleanup;
				}

				char addressString[6] = {0};
				snprintf(addressString, sizeof(addressString), "%hu", address);
				
				char beforeValue[6] = {0};
				char afterValue[6] = {0};
				Register val;
				if (inst.d) {
					snprintf(beforeValue, sizeof(beforeValue), "%hu", GET_REG_VAL(inst.w, inst.reg));

					val.byte.lo = ram[address];
					val.byte.hi = ram[address + 1];
					SET_REG_VAL(1, inst.reg, val.word);

					snprintf(afterValue, sizeof(afterValue), "%hu", GET_REG_VAL(inst.w, inst.reg));
				} else {
					Register beforeVal;
					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);

					val.word = GET_REG_VAL(1, inst.reg);
					ram[address] = val.byte.lo;
					ram[address + 1] = val.byte.hi;

					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);
				}

				if (fprintf(outputFile, " ; %s, %s: %s -> %s", dst, addressString, beforeValue, afterValue) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// mov imm to r/m
		else if (inst.opcode == 0b1100011) {

			inst.w = ram[ip] & 1;
			ip += 1;

			inst.mod = ram[ip] >> 6;
			inst.rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			i32 disp;
			if (sim) {
				disp = getDisplacement(&ram[ip], inst.mod, inst.rm);
				if (disp == -1) {
					fprintf(stderr, "Error getting displacement value\n");
					goto cleanup;
				}
			}

			i8 ipOffset = writeRmString(rmString, &ram[ip], inst.mod, inst.rm, inst.w);
			if (ipOffset == -1) {
				fprintf(stderr, "Error writing rm string to buffer\n");
				goto cleanup;
			}
			ip += (u16)ipOffset;

			u16 imm;
			if (inst.w) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			if (inst.mod == 0 && inst.rm == 0b110) {
				snprintf(immString, sizeof(immString), "%u", imm);
			} else {
				snprintf(immString, sizeof(immString), "%s %u", sizeSpecifier[inst.w], imm);
			}

			if (fprintf(outputFile, "mov %s %s, %s \n", sizeSpecifier[inst.w], rmString, immString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			// TODO: write to output simulation comment
			if (sim) {
				i32 address = getAddress(registersW1, (u16)disp, inst.mod, inst.rm);
				if (address == -1) {
					fprintf(stderr, "Error calculating address\n");
					goto cleanup;
				}

				Register splitImm = { .word = imm };
				ram[address] = splitImm.byte.lo;
				ram[address + 1] = splitImm.byte.hi;
			}
		}
		// mov imm to reg
		else if (inst.opcode == 0b1011) {

			inst.w = (ram[ip] >> 3) & 1;
			inst.reg = ram[ip] & 0b111;
			ip += 1;

			char regString[3] = {0};
			char immString[6] = {0};

			u16 imm;
			if (inst.w) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			strncpy(regString, getRegString(inst.reg, inst.w), 3);

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			if (fprintf(outputFile, "mov %s, %s", regString, immString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			char beforeRegValue[6] = {0};
			char afterRegValue[6] = {0};

			snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(inst.w, inst.reg));
			SET_REG_VAL(inst.w, inst.reg, imm);
			snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(inst.w, inst.reg));

			if (fprintf(outputFile, " ; %s: %s -> %s \n", regString, beforeRegValue, afterRegValue) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// mov memory to accumulator
		else if (inst.opcode == 0b1010000) {

			inst.w = ram[ip] & 1;
			ip += 1;

			inst.addr = (u16)ram[ip] | (u16)(ram[ip + 1] << 8);
			ip += 2;

			char addrString[8] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			snprintf(addrString, sizeof(addrString), "[%hu]", inst.addr);

			if (fprintf(outputFile, "mov %s, %s\n", (char *)accumulatorString[inst.w], addrString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// mov accumulator to memory
		else if (inst.opcode == 0b1010001) {

			inst.w = ram[ip] & 1;
			ip += 1;

			inst.addr = (u16)ram[ip] | (u16)(ram[ip + 1] << 8);
			ip += 2;

			char addrString[8] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			snprintf(addrString, sizeof(addrString), "[%hu]", inst.addr);

			if (fprintf(outputFile, "mov %s, %s\n", addrString, (char *)accumulatorString[inst.w]) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// mov rm to sr
		else if (inst.opcode == 0b10001110) {

			ip += 1;

			inst.mod = ram[ip] >> 6;
			inst.sr = (ram[ip] >> 3) & 0b11;
			inst.rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			i8 ipOffset = writeRmString(rmString, &ram[ip], inst.mod, inst.rm, 1);
			if (ipOffset == -1) {
				fprintf(stderr, "Error: Failed to write rm string to buffer\n");
				goto cleanup;
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(inst.sr), 3);

			if (fprintf(outputFile, "mov %s, %s", srString, rmString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (inst.mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", *segRegisters[inst.sr]);
				*segRegisters[inst.sr] = GET_REG_VAL(1, inst.rm);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", *segRegisters[inst.sr]);

				if (fprintf(outputFile, " ; %s: %s -> %s", srString, beforeRegValue, afterRegValue) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// mov sr to rm
		else if (inst.opcode == 0b10001100) {

			ip += 1;

			inst.mod = ram[ip] >> 6;
			inst.sr = (ram[ip] >> 3) & 0b11;
			inst.rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			i8 ipOffset = writeRmString(rmString, &ram[ip], inst.mod, inst.rm, 1);
			if (ipOffset == -1) {
				fprintf(stderr, "Error writing rm string to buffer\n");
				goto cleanup;
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(inst.sr), 3);

			if (fprintf(outputFile, "mov %s, %s", rmString, srString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && inst.mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(1, inst.rm));
				SET_REG_VAL(1, inst.rm, *segRegisters[inst.sr]);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(1, inst.rm));

				if (fprintf(outputFile, " ; %s: %s -> %s", rmString, beforeRegValue, afterRegValue) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// add/sub/cmp r/m with/and reg to either
		else if ((ram[ip] & 0b11000100) == 0) {

			inst.arithOpcode = ram[ip] >> 3;
			inst.d = (ram[ip] >> 1) & 1;
			inst.w = ram[ip] & 1;
			ip += 1;

			inst.mod = ram[ip] >> 6;
			inst.reg = (ram[ip] >> 3) & 0b111;
			inst.rm = ram[ip] & 0b111;
			ip += 1;

			char regString[3] = {0};
			char rmString[MAX_OPERAND] = {0};
			char arithMnemonic[4] = {0};
			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};

			i32 disp;
			if (sim) {
				disp = getDisplacement(&ram[ip], inst.mod, inst.rm);
				if (disp == -1) {
					fprintf(stderr, "Error getting displacement value\n");
					goto cleanup;
				}
			}

			strncpy(arithMnemonic, arithMnemonics[inst.arithOpcode], 3);
			strncpy(regString, getRegString(inst.reg, inst.w), 3);

			i8 ipOffset = writeRmString(rmString, &ram[ip], inst.mod, inst.rm, inst.w);
			if (ipOffset == -1) {
				fprintf(stderr, "Error writing rm string to buffer\n");
				goto cleanup;
			}
			ip += (u16)ipOffset;

			if (inst.d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
			}

			if (fprintf(outputFile, "%s %s, %s", arithMnemonic, dst, src) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && inst.mod == 3) {

				u8 dstRegOpcode;
				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 dstRegVal;
				u16 srcRegVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};
				char flagsStringBefore[10] = {0};
				char flagsStringAfter[10] = {0};

				if (inst.d) {
					dstRegOpcode = inst.reg;
					dstRegVal = GET_REG_VAL(inst.w, inst.reg);
					srcRegVal = GET_REG_VAL(inst.w, inst.rm);
				} else {
					dstRegOpcode = inst.rm;
					dstRegVal = GET_REG_VAL(inst.w, inst.rm);
					srcRegVal = GET_REG_VAL(inst.w, inst.reg);
				}

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", dstRegVal);

				result = doArithmetic(dstRegVal, srcRegVal, &flagsRegister, &inst);
				SET_REG_VAL(inst.w, dstRegOpcode, result);

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(inst.w, dstRegOpcode));

				sPrintFlags(flagsStringBefore, flagsRegBefore);
				sPrintFlags(flagsStringAfter, flagsRegister);
				if (fprintf(outputFile, " ; %s: %s -> %s | Flags: %s -> %s", rmString, beforeRegValue, afterRegValue, flagsStringBefore, flagsStringAfter) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (sim && inst.w && inst.mod != 3) {
				
				i32 address = getAddress(registersW1, (u16)disp, inst.mod, inst.rm);
				if (address == -1) {
					fprintf(stderr, "Error calculating address\n");
					goto cleanup;
				}

				char addressString[6] = {0};
				snprintf(addressString, sizeof(addressString), "%hu", address);
				
				char beforeValue[6] = {0};
				char afterValue[6] = {0};
				Register val;
				val.byte.lo = ram[address];
				val.byte.hi = ram[address + 1];
				if (inst.d) {
					snprintf(beforeValue, sizeof(beforeValue), "%hu", GET_REG_VAL(inst.w, inst.reg));

					u16 result = doArithmetic(GET_REG_VAL(inst.w, inst.reg), val.word, &flagsRegister, &inst);
					SET_REG_VAL(inst.w, inst.reg, result);

					snprintf(afterValue, sizeof(afterValue), "%hu", GET_REG_VAL(inst.w, inst.reg));
				} else {
					Register beforeVal;
					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);

					u16 result = doArithmetic(val.word, GET_REG_VAL(inst.w, inst.reg), &flagsRegister, &inst);
					val.word = result;
					ram[address] = val.byte.lo;
					ram[address + 1] = val.byte.hi;

					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);
				}

				if (fprintf(outputFile, " ; %s, %s: %s -> %s", dst, addressString, beforeValue, afterValue) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// add/sub/cmp imm to/from/with r/m
		else if (inst.opcode == 0b100000) {

			inst.s = (ram[ip] >> 1) & 1;
			inst.w = ram[ip] & 1;
			ip += 1;

			inst.mod = ram[ip] >> 6;
			inst.arithOpcode = (ram[ip] >> 3) & 0b111;
			inst.rm = ram[ip] & 0b111;
			ip += 1;

			char arithMnemonic[4] = {0};
			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			strncpy(arithMnemonic, arithMnemonics[inst.arithOpcode], 3);

			i8 ipOffset = writeRmString(rmString, &ram[ip], inst.mod, inst.rm, inst.w);
			if (ipOffset == -1) {
				fprintf(stderr, "Error writing rm string to buffer\n");
				goto cleanup;
			}
			ip += (u16)ipOffset;

			u16 imm;
			if (inst.w && !(inst.s)) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			if (inst.s && inst.w) {
				u8 temp = (u8)imm;
				u8 msb = temp >> 7;

				if (msb) {
					imm = (u16)temp | (u16)((0b11111111) << 8);
				} else {
					imm = (u16)temp;
				}
			}

			if (inst.mod == 3) {
				snprintf(immString, sizeof(immString), "%hu", imm);
			} else {
				snprintf(immString, sizeof(immString), "%s %hu", sizeSpecifier[inst.w], imm);
			}

			if (fprintf(outputFile, "%s %s, %s", arithMnemonic, rmString, immString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && inst.mod == 3) {

				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 regVal;
				u16 immVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};
				char flagsStringBefore[10] = {0};
				char flagsStringAfter[10] = {0};

				regVal = GET_REG_VAL(inst.w, inst.rm);
				immVal = (u16)imm;

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", regVal);

				result = doArithmetic(regVal, immVal, &flagsRegister, &inst);
				SET_REG_VAL(inst.w, inst.rm, result);

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(inst.w, inst.rm));

				sPrintFlags(flagsStringBefore, flagsRegBefore);
				sPrintFlags(flagsStringAfter, flagsRegister);
				if (fprintf(outputFile, " ; %s: %s -> %s | Flags: %s -> %s", rmString, beforeRegValue, afterRegValue, flagsStringBefore, flagsStringAfter) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// add/sub/cmp imm to/from/with accumulator
		else if ((ram[ip] & 0b11000110) == 0b00000100) {

			inst.arithOpcode = (ram[ip] >> 3) & 0b111;
			inst.w = ram[ip] & 1;
			ip += 1;

			char immString[6] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			u16 imm;
			if (inst.w) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			if (fprintf(outputFile, "%s %s, %s", (char *)arithMnemonics[inst.arithOpcode], (char *)accumulatorString[inst.w], immString) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim) {

				u8 msb;
				u8 lsb;
				u8 setLSBbits = 0;
				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 dstRegVal;
				u16 srcRegVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};
				char rmString[3] = {0};
				char flagsStringBefore[10] = {0};
				char flagsStringAfter[10] = {0};

				if (inst.w) {
					strncpy(rmString, "ax", 3);
				} else {
					strncpy(rmString, "al", 3);
				}

				dstRegVal = GET_REG_VAL(inst.w, 0);
				srcRegVal = (u16)imm;

				u8 dstLoNibble = dstRegVal & 0b1111;
				u8 srcLoNibble = srcRegVal & 0b1111;

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", dstRegVal);

				switch (inst.arithOpcode) {
					// add
					case 0: {

						if (inst.w) {

							if ((dstRegVal + srcRegVal) > 65535) {
								flagsRegister |= CF;
							} else {
							flagsRegister &= (u16)~CF;
							}

							if (
								((((i16)dstRegVal) + ((i16)srcRegVal)) > 32767) ||
								((((i16)dstRegVal) + ((i16)srcRegVal)) < -32768)
							) {
								flagsRegister |= OF;
							} else {
								flagsRegister &= (u16)~OF;
							}

						} else {
							if ((dstRegVal + srcRegVal) > 255) {
								flagsRegister |= CF;
							} else {
							flagsRegister &= (u16)~CF;
							}

							if (
								((((i8)dstRegVal) + ((i8)srcRegVal)) > 127) ||
								((((i8)dstRegVal) + ((i8)srcRegVal)) < -128)
							) {
								flagsRegister |= OF;
							} else {
								flagsRegister &= (u16)~OF;
							}
						}

						if ((dstLoNibble + srcLoNibble) > 15) {
							flagsRegister |= AF;
						} else {
							flagsRegister &= (u16)~AF;
						}

						SET_REG_VAL(
							inst.w,
							0,
							(dstRegVal + srcRegVal)
						);

						result = GET_REG_VAL(inst.w, 0);

						break;
					}
					// sub
					case 5: {

						if (inst.w) {
							if (
								((((i16)dstRegVal) - ((i16)srcRegVal)) > 32767) ||
								((((i16)dstRegVal) - ((i16)srcRegVal)) < -32768)
							) {
								flagsRegister |= OF;
							} else {
								flagsRegister &= (u16)~OF;
							}
						} else {
							if (
								((((i8)dstRegVal) - ((i8)srcRegVal)) > 127) ||
								((((i8)dstRegVal) - ((i8)srcRegVal)) < -128)
							) {
								flagsRegister |= OF;
							} else {
								flagsRegister &= (u16)~OF;
							}
						}
						
						if (srcLoNibble > dstLoNibble) {
							flagsRegister |= AF;
						} else {
							flagsRegister &= (u16)~AF;
						}

						if (srcRegVal > dstRegVal) {
							flagsRegister |= CF;
						} else {
							flagsRegister &= (u16)~CF;
						}

						result = dstRegVal - srcRegVal;
						SET_REG_VAL(inst.w, 0, result);
						break;
					}
					// cmp
					case 7: {
						if (inst.w) {
							if (
								((((i16)dstRegVal) - ((i16)srcRegVal)) > 32767) ||
								((((i16)dstRegVal) - ((i16)srcRegVal)) < -32768)
							) {
								flagsRegister |= OF;
							} else {
								flagsRegister &= (u16)~OF;
							}
						} else {
							if (
								((((i8)dstRegVal) - ((i8)srcRegVal)) > 127) ||
								((((i8)dstRegVal) - ((i8)srcRegVal)) < -128)
							) {
								flagsRegister |= OF;
							} else {
								flagsRegister &= (u16)~OF;
							}
						}

						if (srcRegVal > dstRegVal) {
							flagsRegister |= CF;
						} else {
							flagsRegister &= (u16)~CF;
						}

						result = dstRegVal - srcRegVal;
						break;
					}
				}

				if (inst.w) {
					Register splitResult = { .word = result };
					lsb = splitResult.byte.lo;
					msb = splitResult.byte.hi;
				} else {
					lsb = (u8)result;
					msb = lsb;
				}

				while (lsb) {
					setLSBbits += lsb & 1;
					lsb >>= 1;
				}

				if (setLSBbits % 2 == 0) {
					flagsRegister |= PF;
				} else {
					flagsRegister &= (u16)~PF;
				}

				if (result == 0) {
					flagsRegister |= ZF;
				} else {
					flagsRegister &= (u16)~ZF;
				}

				if ((msb >> 7) == 1) {
					flagsRegister |= SF;
				} else {
					flagsRegister &= (u16)~SF;
				}

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(inst.w, 0));

				sPrintFlags(flagsStringBefore, flagsRegBefore);
				sPrintFlags(flagsStringAfter, flagsRegister);
				if (fprintf(outputFile, " ; %s: %s -> %s | Flags: %s -> %s", rmString, beforeRegValue, afterRegValue, flagsStringBefore, flagsStringAfter) < 0) {
					fprintf(stderr, "Error writing to output file\n");
					goto cleanup;
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}
		}
		// je / jz
		else if (inst.opcode == 0b01110100) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jz IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(ZF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jl / jnge
		else if (inst.opcode == 0b01111100) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jl IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(SF) != isFlagSet(OF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jle / jng
		else if (inst.opcode == 0b01111110) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jle IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && ( isFlagSet(ZF) || ((isFlagSet(SF) != isFlagSet(OF)) ))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jb / jnae
		else if (inst.opcode == 0b01110010) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jb IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(CF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jbe / jna
		else if (inst.opcode == 0b01110110) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jbe IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && ((isFlagSet(ZF)) || (isFlagSet(CF)) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jp / jpe
		else if (inst.opcode == 0b01111010) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jp IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(PF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jo
		else if (inst.opcode == 0b01110000) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jo IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(OF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// js
		else if (inst.opcode == 0b01111000) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "js IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(SF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jne / jnz
		else if (inst.opcode == 0b01110101) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jnz IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (!isFlagSet(ZF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jnl / jge
		else if (inst.opcode == 0b01111101) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jge IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (isFlagSet(SF) == isFlagSet(OF) )) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jnle / jg
		else if (inst.opcode == 0b01111111) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jg IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && ((!isFlagSet(ZF)) && (isFlagSet(SF) == isFlagSet(OF) ))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jnb / jae
		else if (inst.opcode == 0b01110011) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jae IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (!isFlagSet(CF))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jnbe / ja
		else if (inst.opcode == 0b01110111) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "ja IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && ((!isFlagSet(CF)) && (!isFlagSet(ZF) ))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jnp / jpo
		else if (inst.opcode == 0b01111011) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jnp IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (!isFlagSet(PF))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jno
		else if (inst.opcode == 0b01110001) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jno IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (!isFlagSet(OF))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// jns
		else if (inst.opcode == 0b01111001) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jns IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (!isFlagSet(SF))) {
				ip = (u16)inst.jumpIP;
			}
		}
		// loop
		else if (inst.opcode == 0b11100010) {
			
			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "loop IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim) {
				cx.word--;
				if (cx.word  != 0) {
					ip = (u16)inst.jumpIP;
				}
			}
		}
		// loopz / loope
		else if (inst.opcode == 0b11100001) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "loopz IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim) {
				cx.word--;
				if ((cx.word  != 0) && isFlagSet(ZF)) {
					ip = (u16)inst.jumpIP;
				}
			}
		}
		// loopnz / loopne
		else if (inst.opcode == 0b11100000) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "loopnz IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim) {
				cx.word -= 1;
				if ((cx.word  != 0) && (!isFlagSet(ZF))) {
					ip = (u16)inst.jumpIP;
				}
			}
		}
		// jcxz
		else if (inst.opcode == 0b11100011) {

			Decode_Conditional_Jump_Or_Loop(&inst, ram, &ip);

			if (fprintf(outputFile, "jcxz IP_%u\n", inst.jumpIP) < 0) {
				fprintf(stderr, "Error writing to output file\n");
				goto cleanup;
			}

			if (sim && (cx.word == 0)) {
				ip = (u16)inst.jumpIP;
			}
		}
		else {
			fprintf(stderr, "Byte: 0x%x\n", ram[ip]);
			fprintf(stderr, "Error: Unknown opcode\n");
			goto cleanup;
		}
	}

	if (fprintf(outputFile, "\n\n; Final Registers:\n\n; General Purpose Registers\n") < 0) {
		fprintf(stderr, "Error writing to output file\n");
		goto cleanup;
	}

	for (u8 i = 0; i < 8; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", GET_REG_VAL(1, i));

		if (fprintf(outputFile, ";\t%s: %s\n", (char *)getRegString(i, 1), regValueString) < 0) {
			fprintf(stderr, "Error writing to output file\n");
			goto cleanup;
		}
	}

	if (fprintf(outputFile, "\n; Segment Registers:\n") < 0) {
		fprintf(stderr, "Error writing to output file\n");
		goto cleanup;
	}

	for (u8 i = 0; i < 4; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", *segRegisters[i]);

		if (fprintf(outputFile, ";\t%s: %s\n", (char *)getSRstring(i), regValueString) < 0) {
			fprintf(stderr, "Error writing to output file\n");
			goto cleanup;
		}
	}

	char flagsString[10] = {0};
	sPrintFlags(flagsString, flagsRegister);

	if (fprintf(outputFile, "\n; IP: %hu\n\n; Flags: %s\n", ip, flagsString) < 0) {
		fprintf(stderr, "Error writing to output file\n");
		goto cleanup;
	}

	if (dump) {
		int dumpFD = open("./bin/dump.data", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (dumpFD == -1) {
			perror("Error opening dump file");
			goto cleanup;
		}

		ssize_t bytesWritten = write(dumpFD, arena.base + 256, (64*4*64));
		if (bytesWritten != (64*4*65)) {
			perror("Error writing to dump file");
		}

		if (close(dumpFD) == -1) {
			perror("Error closing dump file");
		}
	}

cleanup:
	int errorCode = EXIT_SUCCESS;
	if (close(inputFD) == -1) {
		perror("Error closing input file");
		errorCode = EXIT_FAILURE;
	}
	if (fclose(outputFile) == EOF) {
		perror("Error closing output file");
		errorCode = EXIT_FAILURE;
	}
	freeArena(&arena);
	return errorCode;
}
