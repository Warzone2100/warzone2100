#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/utsname.h>


uint32_t gdmpFormatVersion = 1;


int main(int argc, char * argv[])
{
	void * btBuffer[128] = {NULL};
	struct utsname sysInfo;
	char * programCommand = NULL, * programVersion = NULL, * compileDate = NULL;
	uint32_t btSize = 0, programCommandSize = 0, programVersionSize = 0, compileDateSize = 0, signum = NSIG, validUname = 0, gdmpVersion = 0;
	uint32_t sizeOfVoidP = 0, sizeOfUtsname = 0, sizeOfChar = 0;

	if (argc != 3)
	{
		printf("USAGE: %s <dumpfile> <binary>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char * dumpFileName = argv[1], addr2line[10000] = {'\0'};
	sprintf( addr2line, "addr2line -e %s ", argv[2]);
	uint32_t addr2lineCommandLength = strlen(addr2line);

	FILE * dumpFile = fopen(dumpFileName, "r");

	if (!dumpFile)
	{
		printf("Could not open %s\n", dumpFileName);
		return EXIT_FAILURE;
	}


	fread(&gdmpVersion, sizeof(uint32_t), 1, dumpFile);
	printf("Examining version %u gdmp...\n\n", gdmpVersion);

	if (gdmpVersion != gdmpFormatVersion)
	{
		printf("Unsupported gdmp version! Got: %u Expected: %u\n", gdmpVersion, gdmpFormatVersion);
		return 1;
	}


	fread(&sizeOfChar, sizeof(uint32_t), 1, dumpFile);
	fread(&sizeOfVoidP, sizeof(uint32_t), 1, dumpFile);
	fread(&sizeOfUtsname, sizeof(uint32_t), 1, dumpFile);


	fread(&programCommandSize, sizeof(uint32_t), 1, dumpFile);
	programCommand = malloc(sizeOfChar*programCommandSize);
	fread(programCommand, sizeOfChar, programCommandSize, dumpFile);
	printf("Program command: %s\n", programCommand);

	fread(&programVersionSize, sizeof(uint32_t), 1, dumpFile);
	programVersion = malloc(sizeOfChar*programVersionSize);
	fread(programVersion, sizeOfChar, programVersionSize, dumpFile);
	printf("Version: %s\n", programVersion);

	fread(&compileDateSize, sizeof(uint32_t), 1, dumpFile);
	compileDate = malloc(sizeOfChar*compileDateSize);
	fread(compileDate, sizeOfChar, compileDateSize, dumpFile);
	printf("Compiled on: %s\n\n", compileDate);


	fread(&validUname, sizeof(uint32_t), 1, dumpFile);
	if (!validUname)
		printf("System information may be invalid!\n");

	fread(&sysInfo, sizeOfUtsname, 1, dumpFile);
	printf("Operating system: %s\n", sysInfo.sysname);
	printf("Node name: %s\n", sysInfo.nodename);
	printf("Release: %s\n", sysInfo.release);
	printf("Version: %s\n", sysInfo.version);
	printf("Machine: %s\n", sysInfo.machine);


	fread(&signum, sizeof(uint32_t), 1, dumpFile);
	printf("\nDump caused by signal: ");

	switch(signum)
	{
		case SIGFPE:
			printf("SIGFPE, Floating point exception\n");
			break;
		case SIGILL:
			printf("SIGILL, Illegal instruction\n");
			break;
		case SIGSEGV:
			printf("SIGSEGV, Segmentation fault\n");
			break;
		case SIGBUS:
			printf("SIGBUS, Bus error\n");
			break;
		case SIGABRT:
			printf("SIGABRT, Abort\n");
			break;
		case SIGTRAP:
			printf("SIGTRAP, Debugger trap\n");
			break;
		case SIGSYS:
			printf("SIGSYS, Bad system call\n");
			break;
		default:
			printf("Unknown signal\n");
			break;
	}


	fread(&btSize, sizeof(uint32_t), 1, dumpFile);
	fread(&btBuffer, sizeOfVoidP, btSize, dumpFile);
	printf("\nBacktrace:\n");

	uint32_t i;
	for(i = 0; i < btSize; i++)
	{
		printf("%16p\n", btBuffer[i]);
		sprintf(addr2line + addr2lineCommandLength + i*16, "%16p", btBuffer[i]);
	}

	printf("\nAt lines:\n");
	system(addr2line);


	fclose(dumpFile);
	free(programCommand);
	free(programVersion);
	free(compileDate);

	return 0;
}
