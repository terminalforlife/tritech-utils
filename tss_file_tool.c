/*
 * Tritech Service System miscellaneous file tool
 * Copyright (C) by Jody Bruchon <jody@jodybruchon.com>
 * This tool handles simple file/device operations that are
 * too clumsy to handle in shell scripts without making the
 * author look really silly ;-)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "version.h"

#define CHECK_IF_NTFS(a)    !strncmp((char *)(a + 3), "NTFS", 4)
#define CHECK_IF_HFSPLUS(a) !strncmp((char *)(a + 1032), "HFSJ", 4)
#define CHECK_IF_EXFAT(a)   !strncmp((char *)(a + 3), "EXFAT   ", 8)
#define CHECK_IF_FVEFS(a)   !strncmp((char *)(a + 3), "-FVE-FS-", 8)

static inline int change_ntfs_geometry(FILE *fp, uint8_t * const restrict bp, const uint8_t heads)
{
	*(bp + 26) = heads;
	fseek(fp, 0, SEEK_SET);
	fwrite(bp, 512, 1, fp);
	if (ferror(fp) != 0) return 1;
	else return 0;
}

int main(const int argc, const char **argv)
{
	FILE *fp;
	uint8_t buffer[4096];
	uint8_t heads;

	if (argc < 3 || argc > 4) goto usage;

	fp = fopen(argv[2], "r+");
	if (!fp) {
		fprintf(stderr, "Can't open %s\n",argv[2]);
		return EXIT_FAILURE;
	}

	fread(buffer, 4096, 1, fp);
	if (ferror(fp) != 0) {
		fprintf(stderr, "Error reading first 4096 bytes of %s\n",argv[1]);
		return EXIT_FAILURE;
	}

	if (!strcmp(argv[1], "ntfs")) {
		if (CHECK_IF_NTFS(buffer)) printf("yes\n");
		else printf("no\n");
		goto finished;
	}
	if (!strcmp(argv[1], "fvefs")) {
		if (CHECK_IF_FVEFS(buffer)) printf("yes\n");
		else printf("no\n");
		goto finished;
	}
	if (!strcmp(argv[1], "ntfsgeom")) {
		if (argc != 4) goto usage;
		/* Convert heads from command line and verify validity */
		if (strlen(argv[3]) != 2) goto usage;
		heads = strtol(argv[3], NULL, 16);
		if (!heads) {
			fprintf(stderr, "Invalid head count specified: %s\n", argv[3]);
			return EXIT_FAILURE;
		}
		if (!CHECK_IF_NTFS(buffer)) {
			fprintf(stderr, "%s is not an NTFS filesystem.\n", argv[2]);
			return EXIT_FAILURE;
		}
		if (change_ntfs_geometry(fp, buffer, heads)) {
			fprintf(stderr, "Error writing to %s\n", argv[2]);
			return EXIT_FAILURE;
		} else {
			printf("Geometry change for %s: %d heads\n", argv[2], heads);
			goto finished;
		}
	}
	if (!strcmp(argv[1], "gpt")) {
		if (buffer[450] == 0xEE) printf("yes\n");
		else printf("no\n");
		goto finished;
	}
	if (!strcmp(argv[1], "winexec")) {
		if (buffer[0] == 0x4D && buffer[1] == 0x5A) printf("%s\n", argv[2]);
		else printf("not_winexec\n");
		goto finished;
	}
	if (!strcmp(argv[1], "registry")) {
		if (!strncmp((char *)buffer, "regf", 4)) printf("yes\n");
		else printf("no\n");
		goto finished;
	}
	if (!strcmp(argv[1], "hfsplus")) {
		if (CHECK_IF_HFSPLUS(buffer)) printf("yes\n");
		else printf("no\n");
		goto finished;
	}
	if (!strcmp(argv[1], "hfsplus_info")) {
		if (!CHECK_IF_HFSPLUS(buffer)) {
			fprintf(stderr, "%s is not an HFS+ filesystem\n", argv[2]);
			exit(EXIT_FAILURE);
		}
		printf("not written\n");
		goto finished;
	}
	if (!strcmp(argv[1], "exfat")) {
		if (CHECK_IF_EXFAT(buffer)) printf("yes\n");
		else printf("no\n");
		goto finished;
	}
	printf("Unknown command %s\n", argv[1]);
	fclose(fp);
	return EXIT_FAILURE;
finished:
	fclose(fp);
	return EXIT_SUCCESS;

usage:
	fprintf(stderr, "Tritech Service System miscellaneous file tool %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
	fprintf(stderr, "Usage: %s command dev/device|/path/to/file [hex-head-count]\n\n", argv[0]);
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "gpt         Checks for GPT partitioning\n");
	fprintf(stderr, "ntfs        Checks for an NTFS filesystem signature\n");
	fprintf(stderr, "fvefs       Checks for a BitLocker (FVE-FS) signature\n");
	fprintf(stderr, "hfsplus     Checks for an HFS+ filesystem signature\n");
	fprintf(stderr, "exfat       Checks for an exFAT filesystem signature\n");
	fprintf(stderr, "winexec     Checks for a DOS/Windows EXE signature\n");
	fprintf(stderr, "ntfsgeom    Change NTFS partition head count; requires two-digit hex head count\n");
	fprintf(stderr, "registry    Checks if the file is a Windows registry hive\n\n");
	return EXIT_FAILURE;
}

