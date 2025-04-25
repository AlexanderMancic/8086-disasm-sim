//  TODO:
//  check if ip is out of bounds
//  maybe change writeoutput to print output
//  check errors for snprintf and strcpy
//  change before and after value from string to u16 / u8

#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "closeFiles.h"
#include "getSRstring.h"
#include "types.h"
#include "writeFlags.h"
// #include "writeOutput.h"
// #include "logFatal.h"
#include "getRegString.h"
#include "constants.h"
#include "writeRMstring.h"
// #include "getImm.h"
#include "register.h"
#include "flagsRegMask.h"

#define DEBUG
#undef DEBUG
#define GET_REG_VAL(w, reg) ((w) ? *registersW1[reg] : *registersW0[reg])
#define SET_REG_VAL(w, reg, val) ((w) ? (*registersW1[reg] = ((u16)val)) : (*registersW0[reg] = ((u8)val)))
#define CLOSE_ALL_FILES (closeFiles(inputFD, disasmOutputFile, simOutputFile))

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

	if (argc != 4) {
		fprintf(stderr, "Usage: %s <infile> <disassembly outfile> <simulation outfile>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int inputFD = open(argv[1], O_RDONLY);
	if (inputFD == -1) {
		perror("Error: Failed to open input file");
		return EXIT_FAILURE;
	}

	FILE *disasmOutputFile = fopen(argv[2], "w");
	if (disasmOutputFile == NULL) {
		perror("Error: Failed to open disassembly file");
		closeFiles(inputFD, NULL, NULL);
		return EXIT_FAILURE;
	}

	FILE *simOutputFile = fopen(argv[3], "w");
	if (disasmOutputFile == NULL) {
		perror("Error: Failed to open simulation file");
		closeFiles(inputFD, disasmOutputFile, NULL);
		return EXIT_FAILURE;
	}

	const bool disasm = true;
	const bool sim = true;

	if (disasm) {
		i32 bytesWritten = fprintf(disasmOutputFile, "bits 16\n");
		if (bytesWritten != 8) {
			if (bytesWritten < 0) {
				perror("Error: Failed to write to disassembly file");
			} else {
				fprintf(stderr, "Error: Failed to write to disassembly file");
			}
			CLOSE_ALL_FILES;
			return EXIT_FAILURE;
		}
	}

	u32 ip = 0;

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
		CLOSE_ALL_FILES;
		return EXIT_FAILURE;
	}

	u8 ram[1024*1024];
	if ((read(inputFD, &ram, (size_t)inputStat.st_size)) != inputStat.st_size) {
		perror("Error: Failed to read input file");
		CLOSE_ALL_FILES;
		return EXIT_FAILURE;
	}

	while (true) {

		if (disasm) {
			i32 bytesWritten = fprintf(disasmOutputFile, "IP_%hu:\n", ip);
			if (bytesWritten < 6 || bytesWritten > 10) {
				if (bytesWritten < 0) {
					perror("Error: Failed to write to disassembly file");
				} else {
					fprintf(stderr, "Error: Failed to write to disassembly file\n");
				}
				CLOSE_ALL_FILES;
				return EXIT_FAILURE;
			}
		}

		// mov r/m to/from reg
		if (ram[ip] >> 2 == 0b100010) {

			u8 w = ram[ip] & 1;
			u8 d = (ram[ip] >> 1) & 1;

			ip += 1;

			u8 reg = (ram[ip] >> 3) & 0b111;
			u8 mod = ram[ip] >> 6;
			u8 rm = ram[ip] & 0b111;

			char regString[regBufferSize] = {0};
			char rmString[rmBufferSize] = {0};
			char instString[rmRegMovInstBufferSize] = {0};
			char dstString[rmBufferSize] = {0};
			char srcString[rmBufferSize] = {0};
			
			strncpy(regString, getRegString(reg, w), regBufferSize);

			i8 ipOffset = writeRMstring(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				perror("Error: Failed to disassemble RM field");
				CLOSE_ALL_FILES;
				return EXIT_FAILURE;
			}
			ip += (u16)ipOffset;

			if (d) {
				strncpy(dstString, regString, rmBufferSize);
				strncpy(srcString, rmString, rmBufferSize);
			} else {
				strncpy(dstString, rmString, rmBufferSize);
				strncpy(srcString, regString, rmBufferSize);
			

			snprintf(instString, sizeof(instString), "mov %s, %s", dstString, srcString);
			}

			if (disasm) {
				i32 bytesWritten = fprintf(disasmOutputFile, "%s\n", instString);
				if (
					bytesWritten < minRmRegMovInstStringSizeWithNewline ||
					bytesWritten > maxRmRegMovInstStringSizeWithNewline
				) {
					if (bytesWritten < 0) {
						perror("Error: Failed to write to disassembly file");
					} else {
						fprintf(stderr, "Error: Failed to write to disassembly file\n");
					}
					CLOSE_ALL_FILES;
					return EXIT_FAILURE;
				}
			}

			if (sim) {
				if (mod == 3) {
					u16 beforeRegValue;
					u16 afterRegValue;
					char simSuffix[simSuffixBufferSize] = {0};

					if (d) {
						beforeRegValue = GET_REG_VAL(w, reg);
						SET_REG_VAL(w, reg, GET_REG_VAL(w, rm));
						afterRegValue = GET_REG_VAL(w, reg);
					} else {
						beforeRegValue = GET_REG_VAL(w, rm);
						SET_REG_VAL(w, rm, GET_REG_VAL(w, reg));
						afterRegValue = GET_REG_VAL(w, rm);
					}

					
					i32 bytesWritten = fprintf(
						disasmOutputFile,
						"%s ; %s: %hu -> %hu\n",
						instString,
						dstString,
						beforeRegValue,
						afterRegValue
					);
					if (bytesWritten < 11 || bytesWritten > ((MAX_OPERAND - 1) * 2 + 7)) { // replace calc
						if (bytesWritten < 0) {
							perror("Error: Failed to write to disassembly file");
						} else {
							fprintf(stderr, "Error: Failed to write to disassembly file\n");
						}
						CLOSE_ALL_FILES;
						return EXIT_FAILURE;
					}
						snprintf(
							simSuffix,
							sizeof(simSuffix),
							" ; %s: %s -> %s\n",
							dstString,
							beforeRegValue,
							afterRegValue
						);

					if (write(simOutputFD, instString, strlen(instString - 1)) != strlen(instString - 1)) {
						perror("Error: Failed to write to simulation file");
						closeFiles(FDs, 3);
						return EXIT_FAILURE;
					}
					if (write(simOutputFD, simSuffix, strlen(simSuffix)) != strlen(simSuffix)) {
						perror("Error: Failed to write to simulation file");
						closeFiles(FDs, 3);
						return EXIT_FAILURE;
					}
				}
			}
		}
		// mov imm to r/m
		else if (ram[ip] >> 1 == 0b1100011) {

			u8 w = ram[ip] & 1;

			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 rm = ram[ip] & 0b111;

			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			i8 ipOffset = writeRMstring(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				perror("Error: Failed to disassemble RM field");
				closeFiles(FDs, 3);
				return EXIT_FAILURE;
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

			if (mod == 3 || (mod == 0 && rm == 0b110)) {
				snprintf(immString, sizeof(immString), "%u", imm);
			} else {
				snprintf(immString, sizeof(immString), "%s %u", sizeSpecifier[w], imm);
			}

			char instString[(MAX_OPERAND - 1) + 11] = {0};
			snprintf(instString, sizeof(instString), "mov %s, %s\n", rmString, immString);

			
			if (disasm) {

				if ((write(disasmOutputFD, instString, strlen(instString))) != strlen(instString)) {
					perror("Error: Failed to disassembe instruction");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
			}
		}
		// mov imm to reg
		else if (ram[ip] >> 4 == 0b1011) {

			u8 reg = ram[ip] & 0b111;
			u8 w = (ram[ip] >> 3) & 1;

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

			char instString[MAX_OPERAND + 5 + 8] = {0};
			snprintf(instString, sizeof(instString), "mov %s, %hu\n", regString, imm);

			if (disasm) {

				if (write(disasmOutputFD, instString, strlen(instString)) != strlen(instString)) {
					perror("Error: Failed to write to disassembly file");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
			}

			if (sim) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};
				char simSuffix[MAX_OPERAND + 11] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(w, reg));
				SET_REG_VAL(w, reg, imm);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(w, reg));


				snprintf(
					simSuffix,
					sizeof(simSuffix),
					" ; %s: %s -> %s\n",
					regString,
					beforeRegValue,
					afterRegValue
				);

				if (write(simOutputFD, instString, strlen(instString - 1)) != strlen(instString - 1)) {
					perror("Error: Failed to write to simulation file");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
				if (write(simOutputFD, simSuffix, strlen(simSuffix)) != strlen(simSuffix)) {
					perror("Error: Failed to write to simulation file");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
			}
		}
		// mov memory to accumulator
		else if (ram[ip] >> 1 == 0b1010000) {

			u8 w = ram[ip] & 1;
			u16 addr = (u16)ram[ip + 1] | (u16)(ram[ip + 2] << 8);

			ip += 2;

			char instString[17] = {0};
			snprintf(instString, sizeof(instString), "mov %s, [%hu]\n", getRegString(0, w), addr);

			if (disasm) {

				if (write(disasmOutputFD, instString, strlen(instString)) != strlen(instString)) {
					perror("Error: Failed to write to disassembly file");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
			}
		}
		// mov accumulator to memory
		else if (ram[ip] >> 1 == 0b1010001) {

			u8 w = ram[ip] & 1;
			u16 addr = (u16)ram[ip + 1] | (u16)(ram[ip + 2] << 8);

			ip += 2;

			char instString[17] = {0};
			snprintf(instString, sizeof(instString), "mov [%hu], %s\n", addr, getRegString(0, w));

			if (disasm) {

				if (write(disasmOutputFD, instString, strlen(instString)) != strlen(instString)) {
					perror("Error: Failed to write to disassembly file");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
			}
		}
		// mov rm to sr
		else if (ram[ip] == 0b10001110) {

			ip += 1;

			u8 mod = ram[ip] >> 6;
			u8 sr = (ram[ip] >> 3) & 0b11;
			u8 rm = ram[ip] & 0b111;

			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			i8 ipOffset = writeRMstring(rmString, &ram[ip], mod, rm, w);
			if (ipOffset == -1) {
				perror("Error: Failed to disassemble RM field");
				closeFiles(FDs, 3);
				return EXIT_FAILURE;
			}
			ip += (u16)ipOffset;

			strncpy(srString, getSRstring(sr), 3);

			char instString[MAX_OPERAND + 9] = {0};
			snprintf(instString, sizeof(instString), "mov %s, %s", getSRstring(sr), rmString);

			if (disasm) {

				if ((write(disasmOutputFD, instString, strlen(instString))) != strlen(instString)) {
					perror("Error: Failed to disassembe instruction");
					closeFiles(FDs, 3);
					return EXIT_FAILURE;
				}
			}

			if (sim) {

			}
			if (mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", *segRegisters[sr]);
				*segRegisters[sr] = GET_REG_VAL(1, rm);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", *segRegisters[sr]);

				if (writeOutput(inputFD, disasmOutputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, srString) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
			}

			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// mov sr to rm
		else if (instBuffer[0] == 0b10001100) {

			if (read(inputFD, &instBuffer[1], 1) != 1) {
				logFatal(inputFD, disasmOutputFD, "Error reading the input file");
			}
			ip += 1;

			u8 mod = instBuffer[1] >> 6;
			u8 sr = (instBuffer[1] >> 3) & 0b11;
			u8 rm = instBuffer[1] & 0b111;
			char rmString[MAX_OPERAND] = {0};
			char srString[3] = {0};

			if (getRMstring(mod, rm, 1, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, disasmOutputFD, "Error getting the rm string");
			}

			strncpy(srString, getSRstring(sr), 3);

			if (writeOutput(inputFD, disasmOutputFD, "mov ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, rmString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, srString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

			if (mod == 3) {

				char beforeRegValue[6] = {0};
				char afterRegValue[6] = {0};

				snprintf(beforeRegValue, sizeof(beforeRegValue), "%hu", GET_REG_VAL(1, rm));
				SET_REG_VAL(1, rm, *segRegisters[sr]);
				snprintf(afterRegValue, sizeof(afterRegValue), "%hu", GET_REG_VAL(1, rm));

				if (writeOutput(inputFD, disasmOutputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, rmString) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
			}

			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// add/sub/cmp r/m with/and reg to either
		else if ((instBuffer[0] & 0b11000100) == 0) {

			if ((read(inputFD, &instBuffer[1], 1)) != 1) {
				logFatal(inputFD, disasmOutputFD, "Error reading the input file");
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
				logFatal(inputFD, disasmOutputFD, "Error getting the rm string");
			}

			if (d) {
				strncpy(dst, regString, MAX_OPERAND);
				strncpy(src, rmString, MAX_OPERAND);
			} else {
				strncpy(dst, rmString, MAX_OPERAND);
				strncpy(src, regString, MAX_OPERAND);
			}

			if (writeOutput(inputFD, disasmOutputFD, arithMnemonic) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, " ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, dst) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, src) == EXIT_FAILURE) {
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

				if (writeOutput(inputFD, disasmOutputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, rmString) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " | Flags: ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, disasmOutputFD, flagsRegBefore) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, disasmOutputFD, flagsRegister) == EXIT_FAILURE) {
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

			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}

		}
		// add/sub/cmp imm to/from/with r/m
		else if (instBuffer[0] >> 2 == 0b100000) {

			if ((read(inputFD, &instBuffer[1], 1)) != 1) {
				logFatal(inputFD, disasmOutputFD, "Error reading the input file");
			}
			ip += 1;

			u8 s = (instBuffer[0] >> 1) & 1;
			u8 w = instBuffer[0] & 1;
			u8 sw = 1 ? w == 1 && s == 0 : 0;
			u8 mod = instBuffer[1] >> 6;
			u8 rm = instBuffer[1] & 0b111;
			u8 arithOpcode = (instBuffer[1] >> 3) & 0b111;
			u16 imm;
			char arithMnemonic[4] = {0};
			char rmString[MAX_OPERAND] = {0};
			char immString[11] = {0};
			char *sizeSpecifier[2] = {"byte", "word"};

			strncpy(arithMnemonic, arithMnemonics[arithOpcode], 3);

			if (getRMstring(mod, rm, w, rmString, inputFD, &ip) == EXIT_FAILURE) {
				logFatal(inputFD, disasmOutputFD, "Error getting the rm string");
			}

			i32 tempImm = getImm(sw, inputFD, &ip);
			if (tempImm == -1) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
			}

			if (s && w) {
				u8 temp = (u8)tempImm;
				u8 msb = temp >> 7;

				if (msb) {
					imm = (u16)temp | (u16)((0b11111111) << 8);
				} else {
					imm = (u16)temp;
				}
			} else {
				imm = (u16)tempImm;
			}

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			snprintf(immString, sizeof(immString), "%s %u", sizeSpecifier[w], imm);

			if (writeOutput(inputFD, disasmOutputFD, arithMnemonic) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, " ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, rmString) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, immString) == EXIT_FAILURE) {
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

				if (writeOutput(inputFD, disasmOutputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, rmString) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " | Flags: ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, disasmOutputFD, flagsRegBefore) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, disasmOutputFD, flagsRegister) == EXIT_FAILURE) {
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

			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
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
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
			}

			snprintf(immString, sizeof(immString), "%hu", (u16)imm);

			if (writeOutput(inputFD, disasmOutputFD, (char *)arithMnemonics[arithOpcode]) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, " ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, (char *)accumulatorString[w]) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, ", ") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, immString) == EXIT_FAILURE) {
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

				if (writeOutput(inputFD, disasmOutputFD, " ; ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, rmString) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, beforeRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, afterRegValue) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " | Flags: ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, disasmOutputFD, flagsRegBefore) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeOutput(inputFD, disasmOutputFD, " -> ") == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				if (writeFlags(inputFD, disasmOutputFD, flagsRegister) == EXIT_FAILURE) {
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

			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// je / jz
		else if (instBuffer[0] == 0b01110100) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jl / jnge
		else if (instBuffer[0] == 0b01111100) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jl IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jle / jng
		else if (instBuffer[0] == 0b01111110) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jle IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jb / jnae
		else if (instBuffer[0] == 0b01110010) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jb IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jbe / jna
		else if (instBuffer[0] == 0b01110110) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jbe IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jp / jpe
		else if (instBuffer[0] == 0b01111010) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jp IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jo
		else if (instBuffer[0] == 0b01110000) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jo IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// js
		else if (instBuffer[0] == 0b01111000) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "js IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jne / jnz
		else if (instBuffer[0] == 0b01110101) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jnz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnl / jge
		else if (instBuffer[0] == 0b01111101) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jge IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnle / jg
		else if (instBuffer[0] == 0b01111111) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jg IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnb / jae
		else if (instBuffer[0] == 0b01110011) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jae IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnbe / ja
		else if (instBuffer[0] == 0b01110111) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "ja IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jnp / jpo
		else if (instBuffer[0] == 0b01111011) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jnp IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jno
		else if (instBuffer[0] == 0b01110001) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jno IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jns
		else if (instBuffer[0] == 0b01111001) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jns IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// loop
		else if (instBuffer[0] == 0b11100010) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "loop IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// loopz / loope
		else if (instBuffer[0] == 0b11100001) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "loopz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// loopnz / loopne
		else if (instBuffer[0] == 0b11100000) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "loopnz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		// jcxz
		else if (instBuffer[0] == 0b11100011) {

			i32 imm = getImm(0, inputFD, &ip);
			if (imm < 0) {
				logFatal(inputFD, disasmOutputFD, "Error decoding immediate value");
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

			if (writeOutput(inputFD, disasmOutputFD, "jcxz IP_") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, jumpIPstring) == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
			if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
				return EXIT_FAILURE;
			}
		}
		else {
			fprintf(stderr, "Byte: 0x%x\n", instBuffer[0]);
			logFatal(inputFD, disasmOutputFD, "Error: Unknown opcode");
		}
	}

	#undef DEBUG
	#ifdef DEBUG

	ax.word = 9999;
	printf("DEBUG(ax.word): %hu\n", GET_REG_VAL(1, 0));

	#endif

	if (writeOutput(inputFD, disasmOutputFD, "\n\n; Final Registers:\n\n; General Purpose Registers\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	for (u8 i = 0; i < 8; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", GET_REG_VAL(1, i));
		#ifdef DEBUG
			printf("DEBUG: %s\n", regValueString);
		#endif

		if (writeOutput(inputFD, disasmOutputFD, ";\t") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, (char *)getRegString(i, 1)) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, regValueString) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}
	if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, disasmOutputFD, "; Segment Registers:\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	for (u8 i = 0; i < 4; i++) {

		char regValueString[6] = {0};

		snprintf(regValueString, sizeof(regValueString), "%hu", *segRegisters[i]);

		if (writeOutput(inputFD, disasmOutputFD, ";\t") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, (char *)getSRstring(i)) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, ": ") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, regValueString) == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
		if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}

	char ipString[6] = {0};
	snprintf(ipString, sizeof(ipString), "%hu", ip);
	if (writeOutput(inputFD, disasmOutputFD, "\n; IP: ") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, disasmOutputFD, ipString) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (writeOutput(inputFD, disasmOutputFD, "\n; Flags: ") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeFlags(inputFD, disasmOutputFD, flagsRegister) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (writeOutput(inputFD, disasmOutputFD, "\n") == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	if (close(inputFD) == -1) {
		perror("Error closing input file");
		if (close(disasmOutputFD) == -1) {
			perror("Error closing input file");
		}
		return EXIT_FAILURE;
	}
	if (close(disasmOutputFD) == -1) {
		perror("Error closing input file");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
