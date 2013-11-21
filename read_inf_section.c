/*
 * Read an INF file section and output to stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "tritech_utils.h"

#define MAXLEN 1024

void usage(char **argv, int value) {
	fprintf(stderr, "Tritech INF/REG/INI file section reader %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
	fprintf(stderr, "Usage:  %s [-a] filename.xxx section_header\n", argv[0]);
	exit(value);
}

void die(error) {
	fprintf(stderr, "Died with error %d\n", error);
	exit(error);
}

int find_section_header(FILE *fp, char *line) {
	unsigned char buffer[MAXLEN];
	char *res;

	while(1) {
		res = fgets(buffer, MAXLEN, fp);
		buffer[(strlen(buffer) - 1)] = '\0';
		if (res) {
			if (strncasecmp(line, buffer, strlen(line)) == 0) return 0;
		} else return 1;
	}
}

int output_extended_section(FILE *fp, char *line) {
	unsigned char buffer[MAXLEN];
	char *res;
	int buflen;

	while(1) {
		if (fgets(buffer, MAXLEN, fp)) {
			buflen = strlen(buffer);
			if (buffer[(buflen - 2)] == 0x0d) buflen--;
			buffer[(buflen - 1)] = '\0';
			if ((buffer[0] == '[') && (strncasecmp(line, buffer, strlen(line)) != 0)) return 0;
			if (buflen > 1) puts(buffer);
		} else return 1;
	}
}

int output_section(FILE *fp) {
	unsigned char buffer[MAXLEN];
	char *res;
	int buflen;

	while(1) {
		if (fgets(buffer, MAXLEN, fp)) {
			buflen = strlen(buffer);
			if (buffer[(buflen - 2)] == 0x0d) buflen--;
			buffer[(buflen - 1)] = '\0';
			if (buffer[0] != '[') {
				if (buflen > 1) puts(buffer);
			}
			else return 0;
		} else return 1;
	}
}

int main(int argc, char **argv) {
	FILE *fp;
	char *inf_file;
	char line[MAXLEN];
	char *origline;
	int read_more = 0;

	switch (argc) {
	case 4:
		if (strncmp(argv[1], "-a", 3) == 0) {
			read_more = 1;
			inf_file = argv[2];
			origline = argv[3];
		} else usage(argv, 1);
		break;

	case 3:
		inf_file = argv[1];
		origline = argv[2];
		break;

	default:
		usage(argv, 1);
		break;
	}

	errno = 0;
	if ((strncmp(inf_file, "-", 1) == 0) && (strlen(argv[1]) == 1)) {
		fp = stdin;
	} else {
		fp = fopen(inf_file, "r");
		if (errno != 0) die(errno);
	}

	strncpy(line, "[", 2);
	strncat(line, origline, MAXLEN-4);
	if (read_more == 0) strncat(line, "]", 2);

	if (find_section_header(fp, line) != 0) {
		fprintf(stderr, "%s: Section [%s] not found in file %s\n", argv[0], origline, inf_file);
		exit(1);
	}

	switch (read_more) {
	case 0:
		output_section(fp);
		break;
	case 1:
		output_extended_section(fp, line);
		break;
	default:
		fprintf(stderr, "Internal error: bad value %d for read_more\n", read_more);
		exit(255);
		break;
	}
	exit(0);
}
