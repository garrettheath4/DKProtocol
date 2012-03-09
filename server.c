/*
 * server.c
 *
 *  Created on: Feb 25, 2012
 *      Author: davisl
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dkp.h"

int usage(char* progName);

int main(int argc, char *argv[]) {
	if (argc != 1) {
		return usage(argv[0]);
	}
	while(1) {
		int sock = dkp_accept();
		if (sock < 0)
		{
			fprintf(stderr, "ERROR: Invalid socket received from dkp_accept().\n");
			return 2;
		}
		fprintf(stderr, "server main(): Socket = %d\n", sock);
		char data[DATA_SIZE];
		assert(sizeof(data) == DATA_SIZE * sizeof(char));
		int recv_status = dkp_recv(sock, data, DATA_SIZE);
		if (recv_status == SUCCESS) {
			printf("Received: \"%s\"\n", data);
		} else {
			printf("ERROR: dkp_recv returned unsuccessful (%d)\n", recv_status);
		}
		dkp_close(sock);
	}
	return 0;
}

int usage(char* progName) {
	printf("usage: %s\n", progName);
	return 1;
}
