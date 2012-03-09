/*
 * client.c
 *
 *  Created on: Feb 25, 2012
 *      Author: davisl
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dkp.h"

const int STR_SIZE = 128;

int usage(char* progName);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		return usage(argv[0]);
	}
	int sock = dkp_connect(argv[1]);
	if (sock < 0)
	{
		fprintf(stderr, "ERROR: Invalid socket received from dkp_connect().\n");
		return 2;
	}
	printf("Client connected to server at %s on socket %d.\n", argv[1], sock);
	char *line = NULL;
	size_t linecap = 0;
	printf("Please type in text to send to server as a message (then press ENTER):\n--> ");
	//gets(line);
	ssize_t linelen = getline(&line, &linecap, stdin);
	if (linelen < 0) {
		fprintf(stderr, "getline() returned %d.  Possible error?\n", (int)linelen);
		line = "";
		linelen = 0;
	}
	printf("String: \"%s\"\n", line);
	printf("Calculated size of line = %d\n", (int)linelen);
	int error = dkp_send(sock, line, linelen);
	dkp_close(sock);
	if(error == SUCCESS) {
		printf("The client was successful in sending the line.\n");
		return 0;
	} else {
		printf("The client FAILED in sending the line (error %d).\n",
				error);
		return 3;
	}
}

int usage(char* progName) {
	printf("usage: %s <IP.address.of.client>\n", progName);
	return 1;
}
