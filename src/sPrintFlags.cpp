#include <stdlib.h>

#include "sPrintFlags.h"
#include "types.h"

u8 sPrintFlags(char *flagsString, u16 flagsRegister){

	u8 index = 0;

	if ((flagsRegister & 1) == 1) {
		flagsString[index++] = 'C';
	}
	if (((flagsRegister >> 2) & 1) == 1) {
		flagsString[index++] = 'P';
	}
	if (((flagsRegister >> 4) & 1) == 1) {
		flagsString[index++] = 'A';
	}
	if (((flagsRegister >> 6) & 1) == 1) {
		flagsString[index++] = 'Z';
	}
	if (((flagsRegister >> 7) & 1) == 1) {
		flagsString[index++] = 'S';
	}
	if (((flagsRegister >> 8) & 1) == 1) {
		flagsString[index++] = 'T';
	}
	if (((flagsRegister >> 9) & 1) == 1) {
		flagsString[index++] = 'I';
	}
	if (((flagsRegister >> 10) & 1) == 1) {
		flagsString[index++] = 'D';
	}
	if (((flagsRegister >> 11) & 1) == 1) {
		flagsString[index] = 'O';
	}

	return EXIT_SUCCESS;
}
