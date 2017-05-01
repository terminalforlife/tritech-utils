/*
 * Decompressor suggestion tool
 * Copyright (C) by Jody Bruchon <jody@jodybruchon.com>
 * Returns the stream decompressor for an archive based on its extension
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "version.h"


/* Extension-to-decompressor table */
struct ext_t {
	const char *ext;
	const char *decomp;
};

static const struct ext_t ext_table[] = {
	{ "lzo", "lzop" },
	{ "tlz", "lzop" },
	{ "tlzo", "lzop" },
	{ "7z", "7za" },
	{ "xz", "xz" },
	{ "txz", "xz" },
	{ "lzm", "xz" },
	{ "lzma", "xz" },
	{ "gz", "gzip" },
	{ "tgz", "gzip" },
	{ "bz2", "bzip2" },
	{ "tbz", "bzip2" },
	{ "tbz2", "bzip2" },
	{ "tz", "gzip" },
	{ "Z", "gzip" },
	{ "z", "gzip" },
	{ "lz", "lzip" },
	{ "lzip", "lzip" },
	{ "lz4", "lz4" },
	{ "lzjody", "lzjody" },
	{ NULL, 0 }
};


int main(int argc, char **argv)
{
	static const char * restrict ext;
	static const struct ext_t * restrict p;

	if (argc < 2) {
		fprintf(stderr, "Tritech compression extension helper %s (%s)", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
		fprintf(stderr, "Suggests a decompressor for a given file name\n");
		fprintf(stderr, "Usage: %s compressed_file.ext\n", argv[0]);
		return EXIT_FAILURE;
	}
	ext = strrchr(argv[1], '.');
	if (!ext) {
		fprintf(stderr, "File name does not have an extension.\n");
		return EXIT_FAILURE;
	}
	ext++;

	for (p = ext_table; p->ext != NULL; ++p) {
		if (strcmp(p->ext, ext) == 0) {
			printf("%s\n", p->decomp);
			return EXIT_SUCCESS;
		}
	}
	fprintf(stderr, "No decompressor known for extension \"%s\".\n", ext);
	return EXIT_SUCCESS;
}
