/*
 * server.h
 *
 *  Created on: Feb 15, 2012
 *      Author: Garrett
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORTNUM 2343

int main(int argc, char *argv[])
{
    char msg[] = "Hello World !\n";

    struct sockaddr_in dest; /* socket info about the machine connecting to us */
    struct sockaddr_in serv; /* socket info about our server */
    int mysocket;            /* socket used to listen for incoming connections */
    int socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));    /* zero the struct before filling the fields */
    serv.sin_family = AF_INET;         /* set the type of connection to TCP/IP */
    serv.sin_addr.s_addr = INADDR_ANY; /* set our address to any interface */
    serv.sin_port = htons(PORTNUM);    /* set the server port number */

    mysocket = socket(AF_INET, SOCK_STREAM, 0);

    /* bind serv information to mysocket */
    bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));

    /* start listening, allowing a queue of up to 1 pending connection */
    listen(mysocket, 1);
    int consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);

    while(consocket)
    {
        printf("Incoming connection from %s - sending welcome\n", inet_ntoa(dest.sin_addr));
        send(consocket, msg, strlen(msg), 0);
    }

    close(consocket);
    close(mysocket);
    return EXIT_SUCCESS;
}
