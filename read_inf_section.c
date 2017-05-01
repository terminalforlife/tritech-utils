/*
 * Windows INF/INI file format section reader
 * Copyright (C) by Jody Bruchon <jody@jodybruchon.com>
 * Read an INF file section and output to stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "version.h"

#define MAXLEN 1024

static char **global_argv;
/* If UTF-16 detected, set this to 1 */
static int read_more = 0;

static char buffer[MAXLEN];


static void usage(int value)
{
	fprintf(stderr, "Tritech INF/REG/INI file section reader %s (%s)\n",
			TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
	fprintf(stderr, "Usage:  %s [-a] filename.xxx section_header\n",
			global_argv[0]);
	exit(value);
}


static void die(const char * const restrict message, int error)
{
	fprintf(stderr, "Died with error %d: %s\n", error, message);
	exit(error);
}


/* Convert UTF-16 to UTF-8, stripping leading white space
 * This uses a "dirty" technique where null bytes are removed but proper
 * Unicode isn't verified. The current purpose of this program is to
 * retrieve data from INF files that are 99.9999% of the time going to be
 * ASCII characters, so this is okay for this purpose. In the future, this
 * should probably be changed to a proper Unicode conversion routine for
 * more general-purpose use. The Unicode gods frown upon this code!
 */
static void utf16_to_utf8(char * const restrict buf)
{
	int src = 0;
	int dest = 0;
	int strip_space = 1;
	unsigned char * restrict line = (unsigned char *)buf;

	/* Nuke the byte order marker (BOM) on the first line */
	if (line[0] == 0xff && line[1] == 0xfe) src = 2;

	/* Iterate through the array, copying nonzero bytes backwards */
	while (src < (MAXLEN - 1)) {
		unsigned char cc;

		// Avoid dereferencing the array over and over (be careful!)
		cc = line[src];

		/* End the line on CR or LF */
		if (cc == '\n' || cc == '\r') break;
		/* Skip zero bytes */
		if (cc == '\0') {
			/* Two nulls in a row ends the string */
			if (line[src+1] == '\0') break;
			src++;
			continue;
		}
		/* Remove any leading whitespace characters */
		if (strip_space) {
			if (cc == ' ' || cc == '\t') {
				src++;
				continue;
			} else strip_space = 0;
		}
		/* Copy byte down */
		line[dest] = cc;
		dest++; src++;
	}
	/* Null-terminate the final string */
	line[dest] = '\0';

	return;
}


static inline int find_section_header(FILE *fp, const char * const restrict line)
{
	char *res;
	int linelength = strlen(line);

	//fprintf(stderr, "find_section_header: %s\n", line);
	while (1) {
		res = fgets(buffer, MAXLEN, fp);
		if (!res) return 1;

		utf16_to_utf8(buffer);
		if (strncasecmp(line, buffer, linelength) == 0) {
			if (read_more) puts(buffer);
			return 0;
		}
	}
}


static inline void strip_comments(char * const restrict buffer)
{
	int quote = 0;
	int i = 0;

	while (i < MAXLEN) {
		/* Don't strip if inside quote marks */
		if (buffer[i] == '"') {
			if (!quote) quote = 1;
			else quote = 0;
		}

		/* Terminate line at semicolon, replace tabs with spaces */
		if (!quote) {
			if (buffer[i] == ';') {
				buffer[i] = '\0';
				return;
			} else if (buffer[i] == '\t') {
				buffer[i] = ' ';
			}
		}
		i++;
	}
	return;
}


static inline int output_section(FILE *fp, const char * const restrict line)
{
	const int linelength = strlen(line);

	//fprintf(stderr, "output_section: %s\n", line);
	while (1) {
		if (fgets(buffer, MAXLEN, fp)) {
			utf16_to_utf8(buffer);
			/* Strip lines with nothing but comments */
			strip_comments(buffer);

			/* Don't process empty lines */
			if (buffer[0] == '\0') continue;

			if (buffer[0] == ' ') printf("--SPACE--\n");
			if (buffer[0] == '[') {
				if (read_more) {
					/* Read all sections with the same title */
					if (strncasecmp(line, buffer, linelength) == 0) {
						puts(buffer);
						return 2;
					}
					else return 1;
				} else return 0;
			}
			puts(buffer);
		} else return 0;
	}
}


int main(int argc, char **argv)
{
	FILE *fp;
	char *inf_file;
	static char line[MAXLEN];
	char *origline;
	int found_one = 0;
	int i;

	global_argv = argv;

	switch (argc) {
	case 4:
		if (strncmp(argv[1], "-a", 3) == 0) {
			read_more = 1;
			inf_file = argv[2];
			origline = argv[3];
		} else usage(1);
		break;

	case 3:
		inf_file = argv[1];
		origline = argv[2];
		break;

	default:
		usage(1);
		break;
	}

	errno = 0;
	if (*inf_file == '-' && strlen(argv[1]) == 1) {
		fp = stdin;
	} else {
		fp = fopen(inf_file, "r");
		if (errno != 0) die("Failed to open the specified file", errno);
	}

	line[0] = '['; line[1] = '\0';
	strncat(line, origline, MAXLEN-4);
	if (read_more == 0) strncat(line, "]", 2);

	/* Output one section, or all matching section prefixes if '-a' specified */
	while (1) {
		if (find_section_header(fp, line) != 0) {
			if (!found_one) {
				fprintf(stderr, "%s: Section [%s] not found in file %s\n",
						argv[0], origline, inf_file);
				exit(EXIT_FAILURE);
			}
		}
		found_one = 1;

		/* If reading all sections, loop until done */
		i = 2;
		while (i == 2) i = output_section(fp, line);
		if (i == 0) break;
		//if (!read_more) break;
	}

	fclose(fp);
	exit(EXIT_SUCCESS);
}
