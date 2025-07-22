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
#include "getSRstring.h"
#include "types.h"
#include "sPrintFlags.h"
#include "logFatal.h"
#include "getRegString.h"
#include "constants.h"
#include "writeRmString.h"
#include "register.h"
#include "flagsRegMask.h"

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
	if (argc == 4 ) {
		if (strncmp(argv[3], "simulate", 8) != 0) {
			fprintf(stderr, "Usage: %s <infile> <outfile> [simulate]\n", argv[0]);
			return EXIT_FAILURE;
		}
		sim = true;
	} else if (argc != 3) {
		fprintf(stderr, "Usage: %s <infile> <outfile> [simulate]\n", argv[0]);
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

	if (fprintf(outputFile, "bits 16\n") < 0) {
		logFatal(inputFD, outputFile, "Error writing to output file");
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
		perror("Error: Failed to read stats of input file");
		logFatal(inputFD, outputFile, "");
	}

	u8 ram[65536] = {0};
	ssize_t bytesRead = read(inputFD, ram, (size_t)inputStat.st_size);
	if (bytesRead != inputStat.st_size) {
		logFatal(inputFD, outputFile, "Error: Failed to read input file");
	}
	if (bytesRead > 65536) {
		logFatal(inputFD, outputFile, "Error: Input file size must not exceed 65,536 bytes");
	}

	while (ip < bytesRead) {

		if (fprintf(outputFile, "IP_%u:\n", ip) < 0) {
			logFatal(inputFD, outputFile, "Error writing to output file");
		}

		// mov r/m to/from reg
		if (ram[ip] >> 2 == 0b100010) {

			u8 d = (ram[ip] >> 1) & 1;
			u8 w = ram[ip] & 1;
			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 reg = (ram[ip] >> 3) & 0b111;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};
			char regString[MAX_OPERAND] = {0};
			char rmString[MAX_OPERAND] = {0};

			strncpy(regString, getRegString(reg, w), 3);

			i32 disp;
			if (sim) {
				disp = getDisplacement(&ram[ip], mod, rm);
				if (disp == -1) {
					logFatal(inputFD, outputFile, "Error: Failed to get displacement value");
				}
			}

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFile, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			if (d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
			}

			if (fprintf(outputFile, "mov %s, %s", dst, src) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && mod == 3) {
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				if (d) {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, reg));
					SET_REG_VAL(w, reg, GET_REG_VAL(w, rm));
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, reg));
				} else {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, rm));
					SET_REG_VAL(w, rm, GET_REG_VAL(w, reg));
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, rm));
				}

				if (fprintf(outputFile, " ; %s: %s -> %s", dst, beforeRegValue, afterRegValue) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (sim && mod != 3) {
				i32 address = getAddress(registersW1, (u16)disp, mod, rm);
				if (address == -1) {
					logFatal(inputFD, outputFile, "Error: Failed to calculate address");
				}

				char addressString[6] = {0};
				snprintf(addressString, sizeof(addressString), "%hu", address);
				
				char beforeValue[6] = {0};
				char afterValue[6] = {0};
				Register val;
				if (d) {
					snprintf(beforeValue, sizeof(beforeValue), "%hu", GET_REG_VAL(w, reg));

					val.byte.lo = ram[address];
					val.byte.hi = ram[address + 1];
					SET_REG_VAL(1, reg, val.word);

					snprintf(afterValue, sizeof(afterValue), "%hu", GET_REG_VAL(w, reg));
				} else {
					Register beforeVal;
					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);

					val.word = GET_REG_VAL(1, reg);
					ram[address] = val.byte.lo;
					ram[address + 1] = val.byte.hi;

					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);
				}

				if (fprintf(outputFile, " ; %s, %s: %s -> %s", dst, addressString, beforeValue, afterValue) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// mov imm to r/m
		else if (ram[ip] >> 1 == 0b1100011) {

			u8 w = ram[ip] & 1;
			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			i32 disp;
			if (sim) {
				disp = getDisplacement(&ram[ip], mod, rm);
				if (disp == -1) {
					logFatal(inputFD, outputFile, "Error: Failed to get displacement value");
				}
			}

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFile, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			u16 imm;
			if (w) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			if (mod == 0 && rm == 0b110) {
				snprintf(immString, sizeof(immString), "%u", imm);
			} else {
				snprintf(immString, sizeof(immString), "%s %u", sizeSpecifier[w], imm);
			}

			if (fprintf(outputFile, "mov %s %s, %s \n", sizeSpecifier[w], rmString, immString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			// TODO: write to output simulation comment
			if (sim) {
				i32 address = getAddress(registersW1, (u16)disp, mod, rm);
				if (address == -1) {
					logFatal(inputFD, outputFile, "Error: Failed to calculate address");
				}

				Register splitImm = { .word = imm };
				ram[address] = splitImm.byte.lo;
				ram[address + 1] = splitImm.byte.hi;
			}
		}
		// mov imm to reg
		else if (ram[ip] >> 4 == 0b1011) {

			u8 w = (ram[ip] >> 3) & 1;
			u8 reg = ram[ip] & 0b111;
			ip += 1;

			char regString[3] = {0};
			char immString[6] = {0};

			u16 imm;
			if (w) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			strncpy(regString, getRegString(reg, w), 3);

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			if (fprintf(outputFile, "mov %s, %s", regString, immString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			char beforeRegValue[6] = {0};
			char afterRegValue[6] = {0};

			snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, reg));
			SET_REG_VAL(w, reg, imm);
			snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, reg));

			if (fprintf(outputFile, " ; %s: %s -> %s \n", regString, beforeRegValue, afterRegValue) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// mov memory to accumulator
		else if (ram[ip] >> 1 == 0b1010000) {

			u8 w = ram[ip] & 1;
			ip += 1;

			u16 addr = (u16)ram[ip] | (u16)(ram[ip + 1] << 8);
			ip += 2;

			char addrString[8] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			snprintf(addrString, sizeof(addrString), "[%hu]", addr);

			if (fprintf(outputFile, "mov %s, %s\n", (char *)accumulatorString[w], addrString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// mov accumulator to memory
		else if (ram[ip] >> 1 == 0b1010001) {

			u8 w = ram[ip] & 1;
			ip += 1;

			u16 addr = (u16)ram[ip] | (u16)(ram[ip + 1] << 8);
			ip += 2;

			char addrString[8] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			snprintf(addrString, sizeof(addrString), "[%hu]", addr);

			if (fprintf(outputFile, "mov %s, %s\n", addrString, (char *)accumulatorString[w]) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// mov rm to sr
		else if (ram[ip] == 0b10001110) {

			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 sr = (ram[ip] >> 3) & 0b11;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, 1);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFile, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(sr), 3);

			if (fprintf(outputFile, "mov %s, %s", srString, rmString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", *segRegisters[sr]);
				*segRegisters[sr] = GET_REG_VAL(1, rm);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", *segRegisters[sr]);

				if (fprintf(outputFile, " ; %s: %s -> %s", srString, beforeRegValue, afterRegValue) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// mov sr to rm
		else if (ram[ip] == 0b10001100) {

			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 sr = (ram[ip] >> 3) & 0b11;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, 1);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFile, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(sr), 3);

			if (fprintf(outputFile, "mov %s, %s", rmString, srString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(1, rm));
				SET_REG_VAL(1, rm, *segRegisters[sr]);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(1, rm));

				if (fprintf(outputFile, " ; %s: %s -> %s", rmString, beforeRegValue, afterRegValue) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// add/sub/cmp r/m with/and reg to either
		else if ((ram[ip] & 0b11000100) == 0) {

			u8 arithOpcode = ram[ip] >> 3;
			u8 d = (ram[ip] >> 1) & 1;
			u8 w = ram[ip] & 1;
			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 reg = (ram[ip] >> 3) & 0b111;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char regString[3] = {0};
			char rmString[MAX_OPERAND] = {0};
			char arithMnemonic[4] = {0};
			char dst[MAX_OPERAND] = {0};
			char src[MAX_OPERAND] = {0};

			i32 disp;
			if (sim) {
				disp = getDisplacement(&ram[ip], mod, rm);
				if (disp == -1) {
					logFatal(inputFD, outputFile, "Error: Failed to get displacement value");
				}
			}

			strncpy(arithMnemonic, arithMnemonics[arithOpcode], 3);
			strncpy(regString, getRegString(reg, w), 3);

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFile, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			if (d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
			}

			if (fprintf(outputFile, "%s %s, %s", arithMnemonic, dst, src) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && mod == 3) {

				u8 dstRegOpcode;
				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 dstRegVal;
				u16 srcRegVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};
				char flagsStringBefore[10] = {0};
				char flagsStringAfter[10] = {0};

				if (d) {
					dstRegOpcode = reg;
					dstRegVal = GET_REG_VAL(w, reg);
					srcRegVal = GET_REG_VAL(w, rm);
				} else {
					dstRegOpcode = rm;
					dstRegVal = GET_REG_VAL(w, rm);
					srcRegVal = GET_REG_VAL(w, reg);
				}

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", dstRegVal);

				result = doArithmetic(dstRegVal, srcRegVal, &flagsRegister, arithOpcode, w);
				SET_REG_VAL(w, dstRegOpcode, result);

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, dstRegOpcode));

				sPrintFlags(flagsStringBefore, flagsRegBefore);
				sPrintFlags(flagsStringAfter, flagsRegister);
				if (fprintf(outputFile, " ; %s: %s -> %s | Flags: %s -> %s", rmString, beforeRegValue, afterRegValue, flagsStringBefore, flagsStringAfter) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (sim && w && mod != 3) {
				
				i32 address = getAddress(registersW1, (u16)disp, mod, rm);
				if (address == -1) {
					logFatal(inputFD, outputFile, "Error: Failed to calculate address");
				}

				char addressString[6] = {0};
				snprintf(addressString, sizeof(addressString), "%hu", address);
				
				char beforeValue[6] = {0};
				char afterValue[6] = {0};
				Register val;
				val.byte.lo = ram[address];
				val.byte.hi = ram[address + 1];
				if (d) {
					snprintf(beforeValue, sizeof(beforeValue), "%hu", GET_REG_VAL(w, reg));

					u16 result = doArithmetic(GET_REG_VAL(w, reg), val.word, &flagsRegister, arithOpcode, w);
					SET_REG_VAL(w, reg, result);

					snprintf(afterValue, sizeof(afterValue), "%hu", GET_REG_VAL(w, reg));
				} else {
					Register beforeVal;
					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);

					u16 result = doArithmetic(val.word, GET_REG_VAL(w, reg), &flagsRegister, arithOpcode, w);
					val.word = result;
					ram[address] = val.byte.lo;
					ram[address + 1] = val.byte.hi;

					beforeVal.byte.lo = ram[address];
					beforeVal.byte.hi = ram[address + 1];
					snprintf(afterValue, sizeof(afterValue), "%hu", beforeVal.word);
				}

				if (fprintf(outputFile, " ; %s, %s: %s -> %s", dst, addressString, beforeValue, afterValue) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// add/sub/cmp imm to/from/with r/m
		else if (ram[ip] >> 2 == 0b100000) {

			u8 s = (ram[ip] >> 1) & 1;
			u8 w = ram[ip] & 1;
			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 arithOpcode = (ram[ip] >> 3) & 0b111;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char arithMnemonic[4] = {0};
			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			strncpy(arithMnemonic, arithMnemonics[arithOpcode], 3);

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFile, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			u16 imm;
			if (w && !s) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			if (s && w) {
				u8 temp = (u8)imm;
				u8 msb = temp >> 7;

				if (msb) {
					imm = (u16)temp | (u16)((0b11111111) << 8);
				} else {
					imm = (u16)temp;
				}
			}

			if (mod == 3) {
				snprintf(immString, sizeof(immString), "%hu", imm);
			} else {
				snprintf(immString, sizeof(immString), "%s %hu", sizeSpecifier[w], imm);
			}

			if (fprintf(outputFile, "%s %s, %s", arithMnemonic, rmString, immString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && mod == 3) {

				u8 msb;
				u8 lsb;
				u8 setLSBbits = 0;
				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 dstRegVal;
				u16 srcRegVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};
				char flagsStringBefore[10] = {0};
				char flagsStringAfter[10] = {0};

				dstRegVal = GET_REG_VAL(w, rm);
				srcRegVal = (u16)imm;

				u8 dstLoNibble = dstRegVal & 0b1111;
				u8 srcLoNibble = srcRegVal & 0b1111;

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", dstRegVal);

				switch (arithOpcode) {
					// add
					case 0: {

						if (w) {

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
							w,
							rm,
							(dstRegVal + srcRegVal)
						);

						result = GET_REG_VAL(w, rm);

						break;
					}
					// sub
					case 5: {

						if (w) {
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
						SET_REG_VAL(w, rm, result);
						break;
					}
					// cmp
					case 7: {
						if (w) {
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

				if (w) {
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

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, rm));

				sPrintFlags(flagsStringBefore, flagsRegBefore);
				sPrintFlags(flagsStringAfter, flagsRegister);
				if (fprintf(outputFile, " ; %s: %s -> %s | Flags: %s -> %s", rmString, beforeRegValue, afterRegValue, flagsStringBefore, flagsStringAfter) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// add/sub/cmp imm to/from/with accumulator
		else if ((ram[ip] & 0b11000110) == 0b00000100) {

			u8 arithOpcode = (ram[ip] >> 3) & 0b111;
			u8 w = ram[ip] & 1;
			ip += 1;

			char immString[6] = {0};
			const char *const accumulatorString[2] = {"al", "ax"};

			u16 imm;
			if (w) {
				imm = (u16)(ram[ip] | (ram[ip + 1] << 8));
				ip += 2;
			} else {
				imm = (u16)ram[ip];
				ip += 1;
			}

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			if (fprintf(outputFile, "%s %s, %s", (char *)arithMnemonics[arithOpcode], (char *)accumulatorString[w], immString) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			{

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

				if (w) {
					strncpy(rmString, "ax", 3);
				} else {
					strncpy(rmString, "al", 3);
				}

				dstRegVal = GET_REG_VAL(w, 0);
				srcRegVal = (u16)imm;

				u8 dstLoNibble = dstRegVal & 0b1111;
				u8 srcLoNibble = srcRegVal & 0b1111;

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", dstRegVal);

				switch (arithOpcode) {
					// add
					case 0: {

						if (w) {

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
							w,
							0,
							(dstRegVal + srcRegVal)
						);

						result = GET_REG_VAL(w, 0);

						break;
					}
					// sub
					case 5: {

						if (w) {
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
						SET_REG_VAL(w, 0, result);
						break;
					}
					// cmp
					case 7: {
						if (w) {
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

				if (w) {
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

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, 0));

				sPrintFlags(flagsStringBefore, flagsRegBefore);
				sPrintFlags(flagsStringAfter, flagsRegister);
				if (fprintf(outputFile, " ; %s: %s -> %s | Flags: %s -> %s", rmString, beforeRegValue, afterRegValue, flagsStringBefore, flagsStringAfter) < 0) {
					logFatal(inputFD, outputFile, "Error writing to output file");
				}
			}

			if (fprintf(outputFile, "\n") < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}
		}
		// je / jz
		else if (ram[ip] == 0b01110100) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jz IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(ZF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jl / jnge
		else if (ram[ip] == 0b01111100) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jl IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(SF) != isFlagSet(OF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jle / jng
		else if (ram[ip] == 0b01111110) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jle IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && ( isFlagSet(ZF) || ((isFlagSet(SF) != isFlagSet(OF)) ))) {
				ip = (u16)jumpIP;
			}
		}
		// jb / jnae
		else if (ram[ip] == 0b01110010) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jb IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(CF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jbe / jna
		else if (ram[ip] == 0b01110110) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jbe IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && ((isFlagSet(ZF)) || (isFlagSet(CF)) )) {
				ip = (u16)jumpIP;
			}
		}
		// jp / jpe
		else if (ram[ip] == 0b01111010) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jp IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(PF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jo
		else if (ram[ip] == 0b01110000) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jo IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(OF) )) {
				ip = (u16)jumpIP;
			}
		}
		// js
		else if (ram[ip] == 0b01111000) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "js IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(SF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jne / jnz
		else if (ram[ip] == 0b01110101) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jnz IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (!isFlagSet(ZF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jnl / jge
		else if (ram[ip] == 0b01111101) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jge IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (isFlagSet(SF) == isFlagSet(OF) )) {
				ip = (u16)jumpIP;
			}
		}
		// jnle / jg
		else if (ram[ip] == 0b01111111) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jg IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && ((!isFlagSet(ZF)) && (isFlagSet(SF) == isFlagSet(OF) ))) {
				ip = (u16)jumpIP;
			}
		}
		// jnb / jae
		else if (ram[ip] == 0b01110011) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jae IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (!isFlagSet(CF))) {
				ip = (u16)jumpIP;
			}
		}
		// jnbe / ja
		else if (ram[ip] == 0b01110111) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "ja IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && ((!isFlagSet(CF)) && (!isFlagSet(ZF) ))) {
				ip = (u16)jumpIP;
			}
		}
		// jnp / jpo
		else if (ram[ip] == 0b01111011) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jnp IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (!isFlagSet(PF))) {
				ip = (u16)jumpIP;
			}
		}
		// jno
		else if (ram[ip] == 0b01110001) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jno IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (!isFlagSet(OF))) {
				ip = (u16)jumpIP;
			}
		}
		// jns
		else if (ram[ip] == 0b01111001) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jns IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (!isFlagSet(SF))) {
				ip = (u16)jumpIP;
			}
		}
		// loop
		else if (ram[ip] == 0b11100010) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "loop IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim) {
				cx.word--;
				if (cx.word  != 0) {
					ip = (u16)jumpIP;
				}
			}
		}
		// loopz / loope
		else if (ram[ip] == 0b11100001) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "loopz IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim) {
				cx.word--;
				if ((cx.word  != 0) && isFlagSet(ZF)) {
					ip = (u16)jumpIP;
				}
			}
		}
		// loopnz / loopne
		else if (ram[ip] == 0b11100000) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "loopnz IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim) {
				cx.word -= 1;
				if ((cx.word  != 0) && (!isFlagSet(ZF))) {
					ip = (u16)jumpIP;
				}
			}
		}
		// jcxz
		else if (ram[ip] == 0b11100011) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			if (fprintf(outputFile, "jcxz IP_%u\n", jumpIP) < 0) {
				logFatal(inputFD, outputFile, "Error writing to output file");
			}

			if (sim && (cx.word == 0)) {
				ip = (u16)jumpIP;
			}
		}
		else {
			fprintf(stderr, "Byte: 0x%x\n", ram[ip]);
			logFatal(inputFD, outputFile, "Error: Unknown opcode");
		}
	}

	if (fprintf(outputFile, "\n\n; Final Registers:\n\n; General Purpose Registers\n") < 0) {
		logFatal(inputFD, outputFile, "Error writing to output file");
	}

	for (u8 i = 0; i < 8; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", GET_REG_VAL(1, i));

		if (fprintf(outputFile, ";\t%s: %s\n", (char *)getRegString(i, 1), regValueString) < 0) {
			logFatal(inputFD, outputFile, "Error writing to output file");
		}
	}

	if (fprintf(outputFile, "\n; Segment Registers:\n") < 0) {
		logFatal(inputFD, outputFile, "Error writing to output file");
	}

	for (u8 i = 0; i < 4; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", *segRegisters[i]);

		if (fprintf(outputFile, ";\t%s: %s\n", (char *)getSRstring(i), regValueString) < 0) {
			logFatal(inputFD, outputFile, "Error writing to output file");
		}
	}

	char flagsString[10] = {0};
	sPrintFlags(flagsString, flagsRegister);

	if (fprintf(outputFile, "\n; IP: %hu\n\n; Flags: %s\n", ip, flagsString) < 0) {
		logFatal(inputFD, outputFile, "Error writing to output file");
	}

	if (close(inputFD) == -1) {
		perror("Error closing input file");
		if (fclose(outputFile) == EOF) {
			perror("Error closing input file");
		}
		return EXIT_FAILURE;
	}
	if (fclose(outputFile) == EOF) {
		perror("Error closing input file");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
