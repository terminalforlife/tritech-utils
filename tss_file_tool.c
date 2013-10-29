/*
 * TSS "is it GPT/NTFS" checking tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tritech-utils.h"

int check_if_ntfs(unsigned char *bp) {
	return strncmp(bp + 3, "NTFS", 4);
}

int change_ntfs_geometry(FILE *fp, unsigned char *bp, unsigned char heads) {
	bp += 26;
	*bp = heads;
	bp -= 26;
	fseek(fp, 0, SEEK_SET);
	fwrite(bp, 512, 1, fp);
	if(ferror(fp) != 0) return 1;
	else return 0;
}

int usage(char **argv) {
		printf("tss_check Version %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
		printf("Usage: %s command dev/device|/path/to/file [hex-head-count]\n\n", argv[0]);
		printf("Commands:\n");
		printf("gpt     \tChecks for GPT partitioning.\n");
		printf("ntfs    \tChecks for an NTFS filesystem signature.\n");
		printf("winexec \tChecks for a DOS/Windows EXE signature.\n");
		printf("ntfsgeom\tChange NTFS partition head count. Requires two-digit hex-head-count.\n");
		exit(1);
}

int main(int argc, char **argv) {
	if(argc < 3 || argc > 4) usage(argv);
	
	FILE *fp;
	unsigned char buffer[512];
	unsigned char *bp = buffer;
	unsigned char heads;
	
	fp = fopen(argv[2], "r+");
	if(!fp) {
		printf("Can't open %s\n",argv[2]);
		exit(1);
	}

	fread(bp, 512, 1, fp);
	if(ferror(fp) != 0) {
		printf("Error reading first sector of %s\n",argv[1]);
		exit(1);
	}

	if(strcmp(argv[1], "ntfs") == 0) {
		if(check_if_ntfs(bp) == 0) {
			puts("yes");
		} else {
			puts("no");
		}
	} else if(strcmp(argv[1], "ntfsgeom") == 0) {
		if(argc != 4) usage(argv);
		/* Convert heads from command line and verify validity */
		if(strlen(argv[3]) != 2) usage(argv);
		heads = strtol(argv[3],NULL,16);
		if(heads < 1) {
			printf("Invalid head count specified: %s\n",argv[3]);
			exit(1);
		}
		if(check_if_ntfs(bp) != 0) {
			printf("%s is not an NTFS filesystem.\n", argv[2]);
			exit(1);
		}
		if(change_ntfs_geometry(fp, bp, heads)) {
			printf("Error writing to %s\n",argv[2]);
			exit(1);
		} else {
			printf("Geometry change for %s: %d heads\n", argv[2], heads);
		}
	} else if(strcmp(argv[1], "gpt") == 0) {
		if(buffer[450] == 0xEE) {
			puts("yes");
		} else {
			puts("no");
		}
	} else if(strcmp(argv[1], "winexec") == 0) {
		if(buffer[0] == 0x4D && buffer[1] == 0x5A) {
			puts(argv[2]);
		} else {
			puts("not_winexec");
		}
	} else {
		printf("Unknown check request: %s\n",argv[1]);
		fclose(fp);
		exit(1);
	}
	fclose(fp);
	exit(0);
}

