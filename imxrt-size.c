#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM1_WARN_PERCENTAGE 99

int printnumbers(unsigned teensy_model_identifier, unsigned flexram_config, 
	unsigned itcm, unsigned dtcm, unsigned ocram, unsigned flash, int stack, unsigned extmem,
	unsigned ocramsize, unsigned flashsize)
{
	int retval = 0;
	unsigned dtcm_allocated = 0;
	unsigned itcm_allocated = 0;		
	unsigned fc = flexram_config;
	for (; fc; fc >>= 2) {
		if ((fc & 3) == 2) {	
			dtcm_allocated += 32;	// 32K per bank;
		}
		else if ((fc & 3) == 3) {
			itcm_allocated += 32;	// 32K per bank;
		}		
	}
	
	unsigned ram1size = (itcm_allocated + dtcm_allocated) * 1024;
	
	printf("RAM1: %6.2f%% of %d kB used.\n", (double)(itcm_allocated*1024 + dtcm) / ram1size * 100, ram1size / 1024);
	//printf("   Code (ITCM):              %6.2f kB\n", itcm / 1024.0);
	printf("   Code (ITCM, 32 kB Blocks):  %6.2f kB\n", itcm / 1024.0);
	//printf("   Code (ITCM):             %6.2f kB (%dx32 kB %s)\n", itcm / 1024.0, itcm_allocated / 32, ((itcm_allocated / 32 > 1) ? "Blocks" : "Block") );
	printf("   Variables (DTCM):           %6.2f kB\n", dtcm / 1024.0);
	printf("   Available for Variables:    %6.2f kB\n", stack / 1024.0);
	printf("\n");
	
	printf("RAM2: %6.2f%% of %d kB used.\n", (double)ocram / ocramsize * 100, ocramsize / 1024);
	printf("   Variables (DMAMEM):         %6.2f kB\n", ocram / 1024.0);
	printf("   Available for Heap:         %6.2f kB\n", (ocramsize - ocram) / 1024.0);
	printf("\n");
	
	if (teensy_model_identifier == 0x25) {
		if (extmem == 0) printf("EXTMEM: not used.\n");
		else printf("EXTMEM: %.2f kB used.\n", extmem / 1024.0);
	        printf("\n");
	}
		
	printf("FLASH: %6.2f%% of %d kB used.\n", (double)flash / flashsize * 100, flashsize / 1024);
	printf("   Code and Constants:         %6.2f kB\n", flash / 1024.0);
	printf("\n");

	//print overflows
	if (stack <= 0) {
		retval = -1;
		printf(">>>>> RAM1 overflowed <<<<<\n\n");
	} 
	else if (stack < ram1size - (ram1size/100.0 * RAM1_WARN_PERCENTAGE) ) {
		printf(">>>>> Warning Stack low <<<<<\n\n");
	}	
	
	if (ocram > ocramsize) {
		retval = -1;
		printf(">>>>> RAM2 overflowed <<<<<\n\n");
	}
	
	if (flash > flashsize) {
		retval = -1;
		printf(">>>>> FLASH overflowed <<<<<\n\n");
	}
	
	return retval;
}

int main() {
	
	const int bl = 200;
	int retval = 0;
	char str[bl + 1];
	char* s;

	unsigned teensy_model_identifier = 0;
	unsigned stext = 0;
	unsigned etext = 0;
	unsigned exidx_end = 0;
	unsigned sdata = 0;
	unsigned ebss = 0;
	unsigned flashimagelen = 0;
	unsigned heap_start = 0;
	unsigned flexram_bank_config = 0;
	unsigned estack = 0;
	unsigned extram_start = 0;
	unsigned extram_end = 0;

	do {
		s = fgets(str, sizeof(str), stdin);
		if (s) {
			str[bl] = 0;
			if (strstr(str, "_teensy_model_identifier")) teensy_model_identifier = strtol(str, NULL, 16);
			if (strstr(str, "T _stext")) stext = strtol(str, NULL, 16);
			if (strstr(str, "T _etext")) etext = strtol(str, NULL, 16);
			if (strstr(str, "R __exidx_end")) exidx_end = strtol(str, NULL, 16);
			if (strstr(str, "D _sdata")) sdata = strtol(str, NULL, 16);
			if (strstr(str, "B _ebss")) ebss = strtol(str, NULL, 16);
			if (strstr(str, " _heap_start")) heap_start = strtol(str, NULL, 16);
			if (strstr(str, " _flashimagelen")) flashimagelen = strtol(str, NULL, 16);
			if (strstr(str, " _estack")) estack = strtoul(str, NULL, 16);
			if (strstr(str, " _flexram_bank_config")) flexram_bank_config = strtoul(str, NULL, 16);
			if (strstr(str, " _extram_start")) extram_start = strtoul(str, NULL, 16);
			if (strstr(str, " _extram_end")) extram_end = strtoul(str, NULL, 16);			
			//puts( str );
		}
	} while (s);

	unsigned itcm = ((exidx_end > etext) ? exidx_end - stext: etext - stext);

	//printf("estack:%x ebss:%x\n", estack, ebss);
	if (teensy_model_identifier == 0x24) { //Teensy40
		retval = printnumbers(teensy_model_identifier, flexram_bank_config, itcm, ebss - sdata, heap_start - 0x20200000, flashimagelen, estack - ebss, 0, 512*1024, 1984*1024);
	}
	else if (teensy_model_identifier == 0x25) {//Teensy41
		retval = printnumbers(teensy_model_identifier, flexram_bank_config, itcm, ebss - sdata, heap_start - 0x20200000, flashimagelen, estack - ebss, extram_end - extram_start, 512*1024, 7936*1024);
	}
	else if (teensy_model_identifier == 0x26) { //TeensyMM
		retval = printnumbers(teensy_model_identifier, flexram_bank_config, itcm, ebss - sdata, heap_start - 0x20200000, flashimagelen, estack - ebss, 0, 512*1024, 16128*1024);
	}
	//else retval = 1;
	return retval;
}
