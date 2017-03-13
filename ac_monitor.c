/*
 * ac_monitor: Watch AC adapter and beep if disconnected
 * Copyright (C) by Jody Bruchon <jody@jodybruchon.com>
 */

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include "tritech_utils.h"

#ifndef CLOCK_TICK_RATE
#define CLOCK_TICK_RATE 1193180
#endif

const char power_path[] = "/sys/class/power_supply/";

/* For toggling beep off when program is killed */
int confd = -1;

/* If interrupted, turn off the beep */
void handle_signal(int signum)
{
	if (signum == SIGINT) {
		if (confd >= 0) {
			ioctl(confd, KIOCSOUND, 0);
			close(confd);
		}
		exit(signum);
	}
}

static inline int do_beep(void)
{
	const struct timespec len = {0, 100000000};
	const int freq1 = CLOCK_TICK_RATE / 1500;
	const int freq2 = CLOCK_TICK_RATE / 3000;
	const int freq3 = CLOCK_TICK_RATE / 6000;

	if ((confd = open("/dev/console", O_WRONLY)) == -1) {
		fprintf(stderr, "Error: could not open /dev/console\n");
		return 1;
	}
	
	/* Beep sequence */
	ioctl(confd, KIOCSOUND, CLOCK_TICK_RATE / freq1);
	nanosleep(&len, NULL);
	ioctl(confd, KIOCSOUND, CLOCK_TICK_RATE / freq2);
	nanosleep(&len, NULL);
	ioctl(confd, KIOCSOUND, CLOCK_TICK_RATE / freq3);
	nanosleep(&len, NULL);
	ioctl(confd, KIOCSOUND, 0);
	close(confd);
	return 0;
}

int main(int argc, char **argv)
{
	static pid_t pid;
	static int i;
	DIR *dir;
	static struct dirent *entry;
	static char path[PATH_MAX], type[PATH_MAX];
	static char buf[16];
	static int status = 0, newstat;

	printf("TSS power monitor %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);

	if (argc >= 2 && strcmp(argv[1], "test") == 0) {
		do_beep();
		exit(EXIT_SUCCESS);
	}

	strncpy(path, power_path, PATH_MAX);

	/* Find the AC adapter file */
	if ((dir = opendir(power_path)) == NULL) {
		fprintf(stderr, "Error: cannot open dir: %s\n", power_path);
		return EXIT_FAILURE;
	}
	errno = 0;
	while ((entry = readdir(dir)) != NULL) {
		if (errno) {
			fprintf(stderr, "Error: readdir() failed: %s\n", power_path);
			exit(EXIT_FAILURE);
		}
		if (*(entry->d_name) != '.') {
			/* Construct the path strings */
			strncat(path, entry->d_name, PATH_MAX);
			strncat(path, "/", PATH_MAX);
			/* "type" is used to determine if this is an AC adapter or not */
			strncpy(type, path, PATH_MAX);
			strncat(type, "type", PATH_MAX);
			if ((i = open(type, O_RDONLY)) != -1) {
				if (read(i, buf, 15) == -1) {
					fprintf(stderr, "Warning: error reading %s\n", type);
					close(i);
				} else {
					if (strncmp(buf, "Mains", 15)) {
						/* "path" is used to check the status of the AC adapter */
						strncat(path, "online", PATH_MAX);
						closedir(dir);
						goto found_ac_file;  /* Found an AC adapter file */
					}
				}
			} else {
				fprintf(stderr, "Warning: error opening %s\n", type);
				close(i);
			}
		}
	}
	closedir(dir);
	fprintf(stderr, "No AC adapters were found in this system.\n");
	exit(EXIT_FAILURE);

found_ac_file:
	/* Get initial adapter state */
	if ((i = open(path, O_RDONLY)) == -1) goto error_open_status;
	if (read(i, buf, 15) == -1) goto error_open_status;
	if (*buf != '0' && *buf != '1') goto error_open_status;
	status = (*buf == '0');
	close(i);

	printf("    AC adapter is currently %s",
		status ? "unplugged.\n" : "plugged in.\n");

	/* Daemonize */
	pid = fork();
	if (pid < 0) exit(EXIT_FAILURE);
	if (pid > 0) exit(EXIT_SUCCESS);
	chdir("/");
	for (i = sysconf(_SC_OPEN_MAX); i > 0; i--) close(i);
	/* Check status every two seconds, beep if changed */
	while (1) {
		sleep(2);
		/* Sleep on errors; they don't necessarily indicate a permanent error */
		if ((i = open(path, O_RDONLY)) == -1) continue;
		if (read(i, buf, 15) == -1) {
			close(i);
			continue;
		}
		close(i);
		if (*buf != '0' && *buf != '1') {
			close(i);
			continue;
		}
		newstat = (*buf == '0');
		if (newstat != status) {
			status = newstat;
			do_beep();
		}
	}
	exit(EXIT_SUCCESS);

error_open_status:
	fprintf(stderr, "Error: cannot open status file: %s\n", path);
	return EXIT_FAILURE;
}
