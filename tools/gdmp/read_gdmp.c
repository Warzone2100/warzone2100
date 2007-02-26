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
	void * btBuffer[128];
	struct utsname sysInfo;
	char * programName = NULL, * programVersion = NULL;
	uint32_t btSize = 0, programNameSize = 0, programVersionSize = 0, signum = NSIG, validUname = 0, gdmpVersion = 0;
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
	if (gdmpVersion != gdmpFormatVersion)
	{
		printf("Unsupported gdmp version! Got: %u Expected: %u\n", gdmpVersion, gdmpFormatVersion);
		return 1;
	}

    fread(&sizeOfChar, sizeof(uint32_t), 1, dumpFile);
    fread(&sizeOfVoidP, sizeof(uint32_t), 1, dumpFile);
    fread(&sizeOfUtsname, sizeof(uint32_t), 1, dumpFile);

    fread(&validUname, sizeof(uint32_t), 1, dumpFile);
    fread(&sysInfo, sizeOfUtsname, 1, dumpFile);

    fread(&programNameSize, sizeof(uint32_t), 1, dumpFile);
	programName = malloc(programNameSize*sizeOfChar);
    fread(programName, sizeOfChar, programNameSize, dumpFile);
    fread(&programVersionSize, sizeof(uint32_t), 1, dumpFile);
	programVersion = malloc(programVersionSize*sizeOfChar);
    fread(programVersion, sizeOfChar, programVersionSize, dumpFile);

    fread(&signum, sizeof(uint32_t), 1, dumpFile);

    fread(&btSize, sizeof(uint32_t), 1, dumpFile);
    fread(&btBuffer, sizeOfVoidP, btSize, dumpFile);

	fclose(dumpFile);

	printf("Examining version %u gdmp...\n\n", gdmpVersion);

	printf("Program name: %s\n", programName);
	printf("Version: %s\n\n", programVersion);

	if (!validUname)
		printf("System information may be invalid!\n");

	printf("Operating system: %s\n", sysInfo.sysname);
	printf("Node name: %s\n", sysInfo.nodename);
	printf("Release: %s\n", sysInfo.release);
	printf("Version: %s\n", sysInfo.version);
	printf("Machine: %s\n", sysInfo.machine);

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

	printf("\nBacktrace:\n");

	int i;
	for(i = 0; i < btSize; i++)
	{
		printf("%#16x\n", btBuffer[i]);
		sprintf(addr2line + addr2lineCommandLength + i*16, "%16x", btBuffer[i]);
	}

	printf("\nAt lines:\n");
	system(addr2line);

	free(programName);
	free(programVersion);

	return 0;
}
