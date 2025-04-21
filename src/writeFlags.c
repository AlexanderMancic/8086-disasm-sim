#include <stdlib.h>

#include "writeFlags.h"
#include "writeOutput.h"

u8 writeFlags(int inputFD, int outputFD, u16 flagsRegister){

	if ((flagsRegister & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "C") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 2) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "P") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 4) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "A") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 6) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "Z") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 7) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "S") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 8) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "T") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 9) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "I") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 10) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "D") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	} if (((flagsRegister >> 11) & 1) == 1) {
		if (writeOutput(inputFD, outputFD, "O") == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
