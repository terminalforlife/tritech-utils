/*
 * TSS "is it GPT/NTFS" checking tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "tritech_utils.h"

#define CHECK_IF_NTFS(a) strncmp((char *)(a + 3), "NTFS", 4)

static inline int change_ntfs_geometry(FILE *fp, uint8_t * const restrict bp, const uint8_t heads)
{
	*(bp + 26) = heads;
	fseek(fp, 0, SEEK_SET);
	fwrite(bp, 512, 1, fp);
	if(ferror(fp) != 0) return 1;
	else return 0;
}

int main(const int argc, const char **argv)
{
	FILE *fp;
	uint8_t buffer[512];
	uint8_t heads;

	if(argc < 3 || argc > 4) goto usage;
	
	fp = fopen(argv[2], "r+");
	if(!fp) {
		printf("Can't open %s\n",argv[2]);
		return EXIT_FAILURE;
	}

	fread(buffer, 512, 1, fp);
	if(ferror(fp) != 0) {
		printf("Error reading first sector of %s\n",argv[1]);
		return EXIT_FAILURE;
	}

	if(!strcmp(argv[1], "ntfs")) {
		if(!CHECK_IF_NTFS(buffer)) puts("yes");
		else puts("no");
	} else if(!strcmp(argv[1], "ntfsgeom")) {
		if(argc != 4) goto usage;
		/* Convert heads from command line and verify validity */
		if(strlen(argv[3]) != 2) goto usage;
		heads = strtol(argv[3],NULL,16);
		if(!heads) {
			printf("Invalid head count specified: %s\n",argv[3]);
			return EXIT_FAILURE;
		}
		if(CHECK_IF_NTFS(buffer)) {
			printf("%s is not an NTFS filesystem.\n", argv[2]);
			return EXIT_FAILURE;
		}
		if(change_ntfs_geometry(fp, buffer, heads)) {
			printf("Error writing to %s\n",argv[2]);
			return EXIT_FAILURE;
		} else {
			printf("Geometry change for %s: %d heads\n", argv[2], heads);
		}
	} else if(!strncmp(argv[1], "gpt", 3)) {
		if(buffer[450] == 0xEE) puts("yes");
		else puts("no");
	} else if(!strncmp(argv[1], "winexec", 7)) {
		if(buffer[0] == 0x4D && buffer[1] == 0x5A) puts(argv[2]);
		else puts("not_winexec");
	} else if(!strncmp(argv[1], "registry", 8)) {
		if(!strncmp((char *)buffer, "regf", 4)) puts("yes");
		else puts("no");
	} else {
		printf("Unknown command %s\n",argv[1]);
		fclose(fp);
		return EXIT_FAILURE;
	}
	fclose(fp);
	return EXIT_SUCCESS;

usage:
	fprintf(stderr, "tss_check Version %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
	fprintf(stderr, "Usage: %s command dev/device|/path/to/file [hex-head-count]\n\n", argv[0]);
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "gpt     \tChecks for GPT partitioning.\n");
	fprintf(stderr, "ntfs    \tChecks for an NTFS filesystem signature.\n");
	fprintf(stderr, "winexec \tChecks for a DOS/Windows EXE signature.\n");
	fprintf(stderr, "ntfsgeom\tChange NTFS partition head count. Requires two-digit hex-head-count.\n");
	fprintf(stderr, "registry\tChecks if the file is a Windows registry hive.\n");
	return EXIT_FAILURE;
}

