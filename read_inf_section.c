/*
 * Read an INF file section and output to stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "tritech_utils.h"

#define MAXLEN 1024

void die(error) {
	fprintf(stderr, "Died with error %d\n", error);
	exit(error);
}

int find_section_header(FILE *fp, char *text) {
	unsigned char buffer[MAXLEN];
	char *res;

	while(1) {
		res = fgets(buffer, MAXLEN, fp);
		buffer[(strlen(buffer) - 1)] = '\0';
		if (res) {
			if (strncasecmp(text, buffer, strlen(text)) == 0) return 0;
		} else return 1;
	}
}

int output_section(FILE *fp) {
	unsigned char buffer[MAXLEN];
	char *res;

	while(1) {
		if (fgets(buffer, MAXLEN, fp)) {
		buffer[(strlen(buffer) - 1)] = '\0';
			if (buffer[0] != '[') {
				if (buffer[0] != '\n') puts(buffer);
			}
			else return 0;
		} else return 1;
	}
}

int main(int argc, char **argv) {
	FILE *fp;
	unsigned char line[MAXLEN];

	if (argc != 3) {
		fprintf(stderr, "Tritech INF/REG/INI file section reader %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
		fprintf(stderr, "Usage:  %s filename.xxx section_header\n", argv[0]);
		exit(1);
	}

	errno = 0;
	if ((strncmp(argv[1], "-", 1) == 0) && (strlen(argv[1]) == 1)) {
		fp = stdin;
	} else {
		fp = fopen(argv[1], "r");
		if (errno != 0) die(errno);
	}

	strncpy(line, "[", 2);
	strncat(line, argv[2], MAXLEN-4);
	strncat(line, "]", 2);

	if (find_section_header(fp, line) != 0) {
		fprintf(stderr, "%s: Section [%s] not found in file %s\n", argv[0], argv[2], argv[1]);
		exit(1);
	}

	output_section(fp);
}
