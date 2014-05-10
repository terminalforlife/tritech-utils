/*
 * ac_monitor: Watch AC adapter and beep if disconnected
 *
 * By Jody Bruchon <jody@jodybruchon.com> 2014-04-23
 *
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
		if(confd >= 0) {
			ioctl(confd, KIOCSOUND, 0);
			close(confd);
		}
		exit(signum);
	}
}

static inline int do_beep(void)
{
	const struct timespec len = {0, 500000000};
	const int freq = 3000;

	if((confd = open("/dev/console", O_WRONLY)) == -1) {
		fprintf(stderr, "Error: could not open /dev/console\n");
		return 1;
	}
	
	/* Beep */
	ioctl(confd, KIOCSOUND, CLOCK_TICK_RATE / freq);
	nanosleep(&len, NULL);
	ioctl(confd, KIOCSOUND, 0);
	close(confd);
	return 0;
}

int main(int argc, char **argv)
{
	pid_t pid;
	int i;
	DIR *dir;
	struct dirent *entry;
	char path[PATH_MAX];
	char type[PATH_MAX];
	char buf[16];
	int status = 0, newstat;

	printf("TSS power monitor version %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
	printf("Copyright (c) by Jody Bruchon\n");
	strncpy(path, power_path, PATH_MAX);

	/* Find the AC adapter file*/
	if((dir = opendir(power_path)) == NULL) {
		fprintf(stderr, "Error: cannot open dir: %s\n", power_path);
		return EXIT_FAILURE;
	}
	errno = 0;
	while((entry = readdir(dir)) != NULL) {
		if(errno) {
			fprintf(stderr, "Error: readdir() failed: %s\n", power_path);
			return EXIT_FAILURE;
		}
		if(*(entry->d_name) != '.') {
			/* Construct the path strings */
			strncat(path, entry->d_name, PATH_MAX);
			strncat(path, "/", PATH_MAX);
			/* "type" is used to determine if this is an AC adapter or not */
			strncpy(type, path, PATH_MAX);
			strncat(type, "type", PATH_MAX);
			if((i = open(type, O_RDONLY)) != -1) {
				if(read(i, buf, 15) == -1) {
					fprintf(stderr, "Warning: error reading %s\n", type);
					close(i);
				} else {
					if(strncmp(buf, "Mains", 15)) {
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
	return EXIT_FAILURE;

found_ac_file:
	/* Get initial adapter state */
	if((i = open(path, O_RDONLY)) == -1) {
		fprintf(stderr, "Error: cannot open status file: %s\n", path);
		return EXIT_FAILURE;
	}
	if (read(i, buf, 15) == -1) return 1;
	if (*buf != '0' && *buf != '1') return 1;
	status = (*buf == '0');
	close(i);

	printf("    AC adapter is currently ");
	if (status) printf("unplugged.\n");
	else printf("plugged in.\n");

	/* Daemonize */
	pid = fork();
	if (pid < 0) return EXIT_FAILURE;
	if (pid > 0) return EXIT_SUCCESS;
	chdir("/");
	for(i = sysconf(_SC_OPEN_MAX); i > 0; i--) close(i);
	/* Check status every second, beep if changed */
	while(1) {
		sleep(1);
		/* Sleep on errors; they don't necessarily indicate a permanent error */
		if((i = open(path, O_RDONLY)) == -1) continue;
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
	return EXIT_SUCCESS;
}
