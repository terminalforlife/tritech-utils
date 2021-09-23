/*
 * Tritech Service System RAID-0 recovery tool
 * Copyright (C) 2021 by Jody Bruchon <jody@jodybruchon.com>
 * Licensed under The MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define BSIZE 16777216

#ifdef ON_WINDOWS
 #define FMODE_R "rbS"
 #define FMODE_W "wbS"
 #undef PTHREADS
#else
 #define FMODE_R "rb"
 #define FMODE_W "wb"
#endif

int main(int argc, char **argv)
{
	FILE **src, *dest;
	char **buf, *dbuf;
	unsigned int stripe, devs, dests, d;
	size_t written;
	off_t outsize = 0, combined = 0;
	int done = 0;
	int status = EXIT_SUCCESS;

	printf("raid0merge: combine 2+ flat RAID-0 disks/images into one disk/image\n\n");

	if (argc < 5) {
		printf("Usage: %s stripe_size drive1 drive2 [drive3 ...] destination\n", argv[0]);
		printf("Copies raw striped (RAID-0) drives to a single destination file or device\n");
		printf("Stripe size is specified in bytes and must be a power of 2\n");
		exit(1);
	}

	/* Number of devices/image files to un-stripe */
	devs = argc - 2;
	dests = argc - 3;

	stripe = (unsigned int)strtoul(argv[1], NULL, 0);
	if (stripe < 512 || stripe > 16777216 || __builtin_popcount(stripe) != 1) goto bad_stripe;

	/* Allocate file pointers and buffers */
	src = (FILE **)malloc(sizeof(FILE *) * devs);
	if (src == NULL) goto oom;
	buf = (char **)malloc(sizeof(char *) * devs);
	if (buf == NULL) goto oom;

	/* Allocate input file buffers and open inputs */
	for (d = 0; d < dests; d++) {
		buf[d] = (char *)malloc(sizeof(char) * BSIZE);
		if (buf[d] == NULL) goto oom;
		src[d] = fopen(argv[d + 2], FMODE_R);
		if (src[d] == NULL) {
			fprintf(stderr, "Can't open input file for reading: %s\n", argv[2 + d]);
			exit(1);
		}
		printf("Input %u: %s\n", d, argv[d + 2]);
	}

	/* Open output file/device and allocate buffer */
	dbuf = (char *)malloc(sizeof(char) * devs * BSIZE);
	if (dbuf == NULL) goto oom;
	dest = fopen(argv[argc - 1], FMODE_W);
	if (dest == NULL) {
		fprintf(stderr, "Can't open output file for reading: %s\n", argv[argc - 1]);
		exit(1);
	}
	printf("Output:  %s\n", argv[argc - 1]);

	/* Debugging info */
/*	printf("stripe %u, popcnt %u, devs %u, files", stripe, __builtin_popcount(stripe), devs);
	for (unsigned int d = 0; d < dests; d++) printf(" %s", argv[d + 2]);
	printf(", out %s\n", argv[argc - 1]); */

	while (done == 0) {
		int spb = BSIZE / stripe;
		outsize = 0;

		/* Read a pile of stripes from each device */
		for (d = 0; d < dests; d++) {
			off_t rc;

			errno = 0;
			rc = fread(buf[d], 1, BSIZE, src[d]);
			/* Stop once we hit an end of file */
			if (rc == 0 && feof(src[d]) == 1) {
				done = 1;
				break;
			}
			outsize += rc;
			if (rc < stripe) {
				fprintf(stderr, "Read less than one full stripe (%ld/%u) from %s\n", rc, stripe, argv[d + 2]);
				status = EXIT_FAILURE;
				goto clean_exit;
			}
		}

		if (outsize == 0) break;

		/* Interlace the stripes together */
		memset(dbuf, 0, BSIZE);
		for (d = 0; d < dests; d++) {
			combined = outsize / dests;
			for (int i = 0; i < spb; i++) {
				memcpy(dbuf + (stripe * i * dests) + (stripe * d), buf[d] + (stripe * i), stripe);
				combined -= stripe;
				if (combined <= 0) break;
			}
		}

		/* Write the combined stripes out */
		written = fwrite(dbuf, 1, outsize, dest);
		if ((off_t)written != outsize) {
			fprintf(stderr, "Failure: only wrote %ld/%ld bytes to output %s\n", written, outsize, argv[argc - 1]);
			status = EXIT_FAILURE;
			goto clean_exit;
		}
}

clean_exit:
	/* Free everything */
	fclose(dest);
	for (d = 0; d < dests; d++) {
		free(buf[d]);
		fclose(src[d]);
	}
	free(buf); free(dbuf); free(src);
	exit(status);

bad_stripe:
	fprintf(stderr, "Stripe must be a power of two between 512 and 16777216\n");
	exit(1);
oom:
	fprintf(stderr, "Out of memory\n");
	exit(1);
}
