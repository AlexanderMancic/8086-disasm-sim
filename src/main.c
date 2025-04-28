#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>

#include "getSRstring.h"
#include "types.h"
#include "writeFlags.h"
#include "writeOutput.h"
#include "logFatal.h"
#include "getRegString.h"
#include "constants.h"
#include "writeRmString.h"
// #include "getImm.h"
#include "register.h"
#include "flagsRegMask.h"

#define DEBUG
#undef DEBUG
#define GET_REG_VAL(w, reg) ((w) ? *registersW1[reg] : *registersW0[reg])
#define SET_REG_VAL(w, reg, val) ((w) ? (*registersW1[reg] = ((u16)val)) : (*registersW0[reg] = ((u8)val)))

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
		logFatal(inputFD, outputFD, "");
	}

	u8 ram[MiB];
	ssize_t bytesRead = read(inputFD, ram, (size_t)inputStat.st_size);
	if (bytesRead != inputStat.st_size) {
		logFatal(inputFD, outputFD, "Error: Failed to read input file");
	}
	if (bytesRead > MiB) {
		logFatal(inputFD, outputFD, "Error: Input file size must not exceed 1 MiB");
	}

	while (ip < bytesRead) {

		char label[16] = {0};
		snprintf(label, sizeof(label), "IP_%u:\n", ip);
		if (writeOutput(inputFD, outputFD, label) == EXIT_FAILURE) {
			return EXIT_FAILURE;
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

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFD, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

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
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, reg));
					SET_REG_VAL(w, reg, GET_REG_VAL(w, rm));
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, reg));
				} else {
					snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, rm));
					SET_REG_VAL(w, rm, GET_REG_VAL(w, reg));
					snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, rm));
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
		else if (ram[ip] >> 1 == 0b1100011) {

			u8 w = ram[ip] & 1;
			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 rm = ram[ip] & 0b111;
			ip += 1;

			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFD, "Error: Failed to write rm string to buffer");
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

			snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, reg));
			SET_REG_VAL(w, reg, imm);
			snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, reg));

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
		else if (ram[ip] >> 1 == 0b1010000) {

			u8 w = ram[ip] & 1;
			ip += 1;

			u16 addr = (u16)ram[ip] | (u16)(ram[ip + 1] << 8);
			ip += 2;

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
		else if (ram[ip] >> 1 == 0b1010001) {

			u8 w = ram[ip] & 1;
			ip += 1;

			u16 addr = (u16)ram[ip] | (u16)(ram[ip + 1] << 8);
			ip += 2;

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
				logFatal(inputFD, outputFD, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(sr), 3);

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

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", *segRegisters[sr]);
				*segRegisters[sr] = GET_REG_VAL(1, rm);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", *segRegisters[sr]);

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
				logFatal(inputFD, outputFD, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(sr), 3);

			if (writeOutput(inputFD, outputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, outputFD, srString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

			if (mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(1, rm));
				SET_REG_VAL(1, rm, *segRegisters[sr]);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(1, rm));

				if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
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

			strncpy(arithMnemonic, arithMnemonics[arithOpcode], 3);
			strncpy(regString, getRegString(reg, w), 3);

			i8 ipOffset = writeRmString(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				logFatal(inputFD, outputFD, "Error: Failed to write rm string to buffer");
			}
			ip += (u16)ipOffset;

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

			if (mod == 3) {

				u8 msb;
				u8 lsb;
				u8 setLSBbits = 0;
				u8 dstRegOpcode;
				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 dstRegVal;
				u16 srcRegVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				if (d) {
					dstRegOpcode = reg;
					dstRegVal = GET_REG_VAL(w, reg);
					srcRegVal = GET_REG_VAL(w, rm);
				} else {
					dstRegOpcode = rm;
					dstRegVal = GET_REG_VAL(w, rm);
					srcRegVal = GET_REG_VAL(w, reg);
				}

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
							dstRegOpcode,
							(dstRegVal + srcRegVal)
						);

						result = GET_REG_VAL(w, dstRegOpcode);

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
						SET_REG_VAL(w, dstRegOpcode, result);
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

				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, dstRegOpcode));

				if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
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
				if (writeOutput(inputFD, outputFD, " | Flags: ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, outputFD, flagsRegBefore) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, outputFD, flagsRegister) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}

				#undef DEBUG
				#ifdef DEBUG
					printf("dst after: %hu\n", GET_REG_VAL(w, dstRegOpcode));
				#endif

				#undef DEBUG
				#ifdef DEBUG
					printf("SUB: SI=%u, DI=%u\n", GET_REG_VAL(1, dstRegOpcode), GET_REG_VAL(1, srcRegOpcode));

					printf("     ODIT SZ A  P C\n");
					u16 value = flagsRegister;
					for (int i = 15; i >= 0; i--) {
						putchar((value & (1 << i)) ? '1' : '0');
						if (i % 4 == 0 && i != 0) putchar(' ');  // Optional: space every 4 bits
					}
					putchar('\n');
				#endif
			}

			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
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
				logFatal(inputFD, outputFD, "Error: Failed to write rm string to buffer");
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

			if (mod == 3) {

				u8 msb;
				u8 lsb;
				u8 setLSBbits = 0;
				u16 flagsRegBefore = flagsRegister;
				u16 result;
				u16 dstRegVal;
				u16 srcRegVal;
				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

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

				if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
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
				if (writeOutput(inputFD, outputFD, " | Flags: ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, outputFD, flagsRegBefore) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, outputFD, flagsRegister) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}

				#undef DEBUG
				#ifdef DEBUG
					printf("dst after: %hu\n", GET_REG_VAL(w, dstRegOpcode));
				#endif

				#undef DEBUG
				#ifdef DEBUG
					printf("SUB: SI=%u, DI=%u\n", GET_REG_VAL(1, dstRegOpcode), GET_REG_VAL(1, srcRegOpcode));

					printf("     ODIT SZ A  P C\n");
					u16 value = flagsRegister;
					for (int i = 15; i >= 0; i--) {
						putchar((value & (1 << i)) ? '1' : '0');
						if (i % 4 == 0 && i != 0) putchar(' ');  // Optional: space every 4 bits
					}
					putchar('\n');
				#endif
			}

			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
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

				if (writeOutput(inputFD, outputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, rmString) == EXIT_FAILURE) {
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
				if (writeOutput(inputFD, outputFD, " | Flags: ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, outputFD, flagsRegBefore) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, outputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, outputFD, flagsRegister) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}

				#undef DEBUG
				#ifdef DEBUG
					printf("dst after: %hu\n", GET_REG_VAL(w, dstRegOpcode));
				#endif

				#undef DEBUG
				#ifdef DEBUG
					printf("SUB: SI=%u, DI=%u\n", GET_REG_VAL(1, dstRegOpcode), GET_REG_VAL(1, srcRegOpcode));

					printf("     ODIT SZ A  P C\n");
					u16 value = flagsRegister;
					for (int i = 15; i >= 0; i--) {
						putchar((value & (1 << i)) ? '1' : '0');
						if (i % 4 == 0 && i != 0) putchar(' ');  // Optional: space every 4 bits
					}
					putchar('\n');
				#endif
			}

			if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// je / jz
		else if (ram[ip] == 0b01110100) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111100) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111110) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110010) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110110) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111010) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110000) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111000) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110101) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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

			if (sim && ((flagsRegister >> 6) & 1) == 0) {
				ip = (u16)jumpIP;
			}
		}
		// jnl / jge
		else if (ram[ip] == 0b01111101) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111111) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110011) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110111) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111011) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01110001) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b01111001) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b11100010) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b11100001) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b11100000) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
		else if (ram[ip] == 0b11100011) {

			ip += 1;

			i8 ip_inc8 = (i8)ram[ip];
			ip += 1;

			i32 jumpIP = ip + ip_inc8;

			char jumpIPstring[11] = {0};
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
			fprintf(stderr, "Byte: 0x%x\n", ram[ip]);
			logFatal(inputFD, outputFD, "Error: Unknown opcode");
		}
	}

	#undef DEBUG
	#ifdef DEBUG

	ax.word = 9999;
	printf("DEBUG(ax.word): %hu\n", GET_REG_VAL(1, 0));

	#endif

	if (writeOutput(inputFD, outputFD, "\n\n; Final Registers:\n\n; General Purpose Registers\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	for (u8 i = 0; i < 8; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", GET_REG_VAL(1, i));
		#ifdef DEBUG
			printf("DEBUG: %s\n", regValueString);
		#endif

		if (writeOutput(inputFD, outputFD, ";\t") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, (char *)getRegString(i, 1)) == EXIT_FAILURE) {
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

		snprintf(regValueString, sizeof(regValueString), "%hu", *segRegisters[i]);

		if (writeOutput(inputFD, outputFD, ";\t") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, outputFD, (char *)getSRstring(i)) == EXIT_FAILURE) {
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

	char ipString[6] = {0};
	snprintf(ipString, sizeof(ipString), "%hu", ip);
	if (writeOutput(inputFD, outputFD, "\n; IP: ") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, outputFD, ipString) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (writeOutput(inputFD, outputFD, "\n; Flags: ") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeFlags(inputFD, outputFD, flagsRegister) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, outputFD, "\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
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
