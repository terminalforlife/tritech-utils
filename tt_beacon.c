/* TSS UDP beacon utility */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "tritech_utils.h"

int main(int argc, char **argv)
{
	int sockfd;
	int broadcast = 1;
	char *address;
	unsigned int port;
	unsigned int interval;
	char *beacon_str;
	struct sockaddr_in dest_addr;

	fprintf(stderr, "Tritech Service System UDP beacon utility %s (%s)\n", TRITECH_UTILS_VER, TRITECH_UTILS_DATE);
	if (argc != 5) {
		fprintf(stderr, "Usage: %s broadcast_addr port interval \"beacon text string\"\n", argv[0]);
		exit(1);
	}

	address = argv[1];
	port = atoi(argv[2]);
	interval = atoi(argv[3]);
	beacon_str = argv[4];

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, address, &(dest_addr.sin_addr)) != 1) {
		perror("inet_pton");
		exit(1);
	}

	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);

	fprintf(stderr, "Sending \"%s\" to addr %s port %d every %d seconds.\n", beacon_str, address, port, interval);

	do {
		if (sendto(sockfd, beacon_str, strlen(beacon_str), MSG_DONTROUTE,
				(struct sockaddr *)&dest_addr, sizeof dest_addr) == -1) {
			perror("sendto");
			exit(1);
		}
		sleep(interval);
	} while (1);

	close(sockfd);

	exit(0);
}
