/*
 * dkp.c
 *
 *  Created on: Feb 25, 2012
 *      Author: davisl
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "dkp.h"
#include "bool.h"

const bool DEBUG_ALL    = FALSE;
const bool DEBUG_CKSUM  = FALSE;
const bool DEBUG_PACKET = TRUE;

#define FLAG_LEN     1
#define CKSUM_LEN   16
#define EMPTY_FIELD -1

const int HEADER_LEN   = FLAG_LEN + CKSUM_LEN;

const int RECONNECT_TIMEOUT = 6;
const int MAX_TRIES         = 10;
const int SOCK_BACKLOG      = 1;

unsigned int RSHash(char* str, unsigned int len);
void printData(char data[]);
void errno_info(int errsv);
void ack_status_info(int ack_status);

int dkp_connect(char ip_addr[])
{
	/*
	struct in_addr {
		unsigned long s_addr;
	};

	struct sockaddr_in {
		short            sin_family;
		unsigned short   sin_port;
		struct in_addr   sin_addr;
		char             sin_zero[8];
	};
	*/

	struct sockaddr_in connect_sock_params;

	//Get a socket descriptor from the system
	//TODO: Change this back to SOCK_DGRAM
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		errno_info(errno);
		printf("ERROR: Unable to allocate a socket from system (error %d).\n",
				sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("Client socket number %d\n", sock);

	/*
	// Check the network interface value of the socket
	char opt[8];
	memset(opt, 0, sizeof(opt));
	unsigned int optlen = sizeof(opt);
	int opt_err = getsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, opt, &optlen);
	int sockopt_errsv = errno;
	if (opt_err < 0) {
		printf("getsockopt() returned %d.\n", opt_err);
		printf("%d: %s\n", sockopt_errsv, strerror(sockopt_errsv));
	} else {
		printf("Socket bound to %s\n", opt);
	}
	*/

	memset(&connect_sock_params, 0, sizeof(connect_sock_params));
	connect_sock_params.sin_family = AF_INET;
	connect_sock_params.sin_port = htons(PORT_NUM);
	connect_sock_params.sin_addr.s_addr = inet_addr(ip_addr);
	if (connect_sock_params.sin_addr.s_addr == INADDR_NONE) {
		printf("ERROR: %d: Invalid IP address.\n", INADDR_NONE);
		dkp_close(sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("IP Address: %s -> %x\n", ip_addr, connect_sock_params.sin_addr.s_addr);

	int connect_status = connect(sock, (struct sockaddr *)&connect_sock_params, sizeof(struct sockaddr));
	if(connect_status < 0)
	{
		errno_info(errno);
		// Error checking
		printf("ERROR: Unable to establish a connection.\n");

		// Final assert statements
		assert(connect_sock_params.sin_family == AF_INET);
		assert(connect_sock_params.sin_port == htons(PORT_NUM));
		assert(connect_sock_params.sin_addr.s_addr == inet_addr(ip_addr));

		// Abort
		dkp_close(sock);
		return NODENOTAVL;
	}

	//Client's advertised window.
	int send_msg1[2];
	send_msg1[0] = SYN;	// Flag
	send_msg1[1] = 1;	// Advertised Window (1 packet)
	//Start handshaking
	// client side
	//0 signifies a SYN Request
	int send1_status = send(sock, send_msg1, sizeof(send_msg1), 0);
	if(send1_status < 0)
	{
		errno_info(errno);
		printf("ERROR: SYN: Initial handshaking failed.\n");
		dkp_close(sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("send1_status = %d\n", send1_status);

	int recv_msg2[2];
	recv_msg2[0] = EMPTY_FIELD;
	recv_msg2[1] = EMPTY_FIELD;
	//1 signifies a SYN+ACK
	int recv2_status = recv(sock, recv_msg2, sizeof(recv_msg2), 0);
	if(recv2_status < 0)
	{
		errno_info(errno);
		printf("recv2_status = %d\n", recv2_status);
		printf("ERROR: SYN_ACK: Step two of handshaking failed.\n");
		dkp_close(sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("dkp_connect() SYN_ACK: %d, %d\n", recv_msg2[0], recv_msg2[1]);
	// Check the client's advertised window size to make sure it's 1.

	int send_msg3[2];
	send_msg3[0] = FIN_ACK;	// Flag
	send_msg3[1] = 1;		// Advertised window
	//2 signifies a Final Ack
	int send3_status = send(sock, send_msg3, sizeof(send_msg3), 0);
	if(send3_status < 0)
	{
		errno_info(errno);
		printf("ERROR: FIN_ACK: Step three of handshaking failed.\n");
		dkp_close(sock);
		return SOCKERROR;
	}

	return sock;
}

int dkp_accept()
{
	// Function variables
	struct sockaddr_in dest_sock_params;
	struct sockaddr_in serv_sock_params;
	unsigned int socksize = sizeof(struct sockaddr_in);

	//Set up the SERVER SOCKET parameters
	memset(&serv_sock_params, 0, sizeof(serv_sock_params));
	serv_sock_params.sin_family = AF_INET;
	serv_sock_params.sin_port = htons(PORT_NUM);
	serv_sock_params.sin_addr.s_addr = INADDR_ANY;
	if (serv_sock_params.sin_addr.s_addr == INADDR_NONE) {
		printf("ERROR: %d: Invalid IP address.\n", INADDR_NONE);
		return SOCKERROR;
	}

	//Handshaking server-side
	//TODO: Change this back to SOCK_DGRAM
	int listening_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listening_sock < 0) {
		errno_info(errno);
		printf("ERROR: Unable to allocate a socket from system.\n");
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("Server is listening on socket number %d\n", listening_sock);

	//Bind the socket
	while (1) {
		int bind_status = bind(listening_sock, (struct sockaddr *)&serv_sock_params, sizeof(struct sockaddr));
		int bind_errno = errno;
		if (bind_status < 0) {
			if (bind_errno == EADDRINUSE) {
				printf("Server socket %d not yet free.  Waiting %d seconds before trying again...\n",
						listening_sock, RECONNECT_TIMEOUT);
				sleep(RECONNECT_TIMEOUT);
				//printf("Server socket not freed by system yet.  Please try again later.\n");
				//return SOCKERROR;
			} else {
				errno_info(bind_errno);
				printf("ERROR: %d: Unable to bind socket %d.\n", bind_status, listening_sock);
				dkp_close(listening_sock);
				return SOCKERROR;
			}
		} else {
			printf("Successfully bound server to socket %d.  "
					"Ready to accept incoming connections...\n", listening_sock);
			break;
		}
	}

	//Tell the socket to listen
	//errno = SUCCESS;
	int listen_err = listen(listening_sock, SOCK_BACKLOG);
	if (listen_err < 0) {
		errno_info(errno);
		printf("ERROR: %d: Unable to listen to the socket.\n", listen_err);
		dkp_close(listening_sock);
		return SOCKERROR;
	}

	//Accept the incoming connection
	int accepted_conn_sock = accept(listening_sock, (struct sockaddr *)&dest_sock_params, &socksize);
	if (accepted_conn_sock < 0)
	{
		errno_info(errno);
		printf("ERROR: %d: Unable to accept incoming connection (not available?).\n",
				accepted_conn_sock);
		dkp_close(listening_sock);
		return NODENOTAVL;
	} else {
		printf("Accepted incoming connection from %s on socket number %d\n",
				inet_ntoa(dest_sock_params.sin_addr), accepted_conn_sock);
	}

	//TODO: Uncomment this section to enable handshaking
	//Server's advertised window

	int recv_msg1[2];
	recv_msg1[0] = EMPTY_FIELD;
	recv_msg1[1] = EMPTY_FIELD;
	if (DEBUG_ALL)	printf("Waiting to receive advertised window\n");
	int recv1_status = recv(accepted_conn_sock, recv_msg1, sizeof(recv_msg1), 0);
	if (recv1_status <= 0)
	{
		errno_info(errno);
		printf("ERROR: recv() -> %d: Receive call failed.\n", recv1_status);
		printf("ERROR: SYN: Step one of handshaking failed.\n");
		dkp_close(listening_sock);
		dkp_close(accepted_conn_sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("recv1_status = %d\n", recv1_status);
	if (DEBUG_ALL)	printf("Received SYN: %d, %d\n", recv_msg1[0], recv_msg1[1]);


	int send_msg2[2];
	send_msg2[0] = SYN_ACK;	// Flag
	send_msg2[1] = 1;		// Advertised Window
	int send2_status = send(accepted_conn_sock, send_msg2, sizeof(send_msg2), 0);
	if(send2_status <= 0)
	{
		errno_info(errno);
		printf("ERROR: SYN_ACK: Step two of handshaking failed.\n");
		dkp_close(listening_sock);
		dkp_close(accepted_conn_sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("send2_status = %d\n", send2_status);

	int recv_msg3[2];
	int recv3_status = recv(accepted_conn_sock, recv_msg3, sizeof(recv_msg3), 0);
	if(recv3_status <= 0)
	{
		errno_info(errno);
		printf("ERROR: FIN_ACK: Step three of handshaking failed.\n");
		dkp_close(listening_sock);
		dkp_close(accepted_conn_sock);
		return SOCKERROR;
	}
	if (DEBUG_ALL)	printf("recv3_status = %d\n", recv3_status);
	if (DEBUG_ALL)	printf("Received FIN_ACK: %d, %d\n", recv_msg3[0], recv_msg3[1]);

	dkp_close(listening_sock);

	return accepted_conn_sock;
}



int dkp_send(int socket_num, char data[], unsigned long int data_size_bytes)
{
	int sending_cksum;
	int i;

	char data_filled[DATA_SIZE];
	memset(&data_filled, 0, sizeof(data_filled));
	for (i=0; i<DATA_SIZE; i++) {
		if (i < data_size_bytes) {
			data_filled[i] = data[i];
		} else {
			data_filled[i] = '\0';
		}
	}

	if (DEBUG_ALL || DEBUG_PACKET)	printData(data_filled);

	sending_cksum = RSHash(data_filled, DATA_SIZE);

	char cksum_str[HEADER_LEN];
	char cksum_format[20];
	sprintf(cksum_format, "%%0%dd", CKSUM_LEN);
	sprintf(cksum_str, cksum_format, sending_cksum);
	printf("Calculated checksum = %d\n", sending_cksum);
	/*
	char padding[HEADER_LEN];
	padding[0] = '\0';	//string padding = "";
	int padding_len = CKSUM_LEN - strlen(cksum_str_short);
	for (; padding_len>0; padding_len--)
	{
		strcat(padding, "0");
	}
	char *cksum_str = strcat(padding, cksum_str_short);
	*/
	char header[HEADER_LEN+1];
	header[0] = '0';	// NORM flag
	header[1] = '\0';
	strcat(header, cksum_str);

	char message[HEADER_LEN + DATA_SIZE];

	// Fill in header in message (don't include '\0' from header[] string)
	for (i=0; i<HEADER_LEN; i++) {
		message[i] = header[i];
	}

	// Fill in data in message
	for (i=HEADER_LEN; i<HEADER_LEN + DATA_SIZE; i++) {
		assert((i - HEADER_LEN) >= 0 && (i - HEADER_LEN) < DATA_SIZE);
		message[i] = data_filled[i - HEADER_LEN];
	}

	int num_sends = 0;
	int ack_status = EMPTY_FIELD;
	do {
		if (DEBUG_ALL) printf("Sending a total of %d bytes of data in message.\n", (HEADER_LEN + DATA_SIZE));
		int send_status = send(socket_num, message, HEADER_LEN + DATA_SIZE, 0);
		int send_errno = errno;
		if (send_status <= 0) {
			errno_info(send_errno);
			printf("ERROR: dkp_send: send() of message unsuccessful (error %d)\n", send_status);
		} else {
			printf("Sent %d bytes of data, but waiting for acknowledgment.\n", send_status);
		}
		int recv_status = recv(socket_num, &ack_status, sizeof(ack_status), 0);
		int recv_errno = errno;
		if (recv_status <= 0) {
			errno_info(recv_errno);
			printf("ERROR: dkp_send: recv() of ACK unsuccessful (error %d)\n", recv_status);
		} else {
			if (DEBUG_ALL)	printf("recv() of ACK returned okay\n");
		}
		ack_status_info(ack_status);
		num_sends++;
	} while ((ack_status == NEG_ACK || ack_status == EMPTY_FIELD) && num_sends < MAX_TRIES);
	if (num_sends < MAX_TRIES) {
		printf("Successfully sent %d bytes of data in %d tries.\n",
				HEADER_LEN + DATA_SIZE, num_sends);
		return SUCCESS;
	} else {
		printf("Too many failed attempts to send %lu bytes of data.  Giving up.\n", data_size_bytes);
		return SOCKERROR;
	}
}

int dkp_recv(int socket_num, char data[], unsigned long int data_size_bytes)
{
	char packet[HEADER_LEN + DATA_SIZE];
	char *cksum_str = malloc((CKSUM_LEN + 1) * sizeof(char));
	int num_receives = 0;

	//printf("data_size_bytes = %lu\n", data_size_bytes);
	//memset(data, 0, data_size_bytes);

	do {
		int recv_len = recv(socket_num, packet, HEADER_LEN + DATA_SIZE, 0);
		int recv_errno = errno;
		if (recv_len <= 0) {
			errno_info(recv_errno);
			printf("ERROR: dkp_recv: recv() of message unsuccessful (error %d)\n", recv_len);
			return SOCKERROR;
		} else {
			printf("Received: \"%s\" (%d bytes).\n", packet, recv_len);
		}

		// Null terminate the received data
		packet[recv_len] = '\0';

		// Peel off flag
		char flag_str[2];
		flag_str[0] = packet[0];
		flag_str[1] = '\0';
		int flag = atoi(flag_str);
		assert(flag == NORM);

		// Peel off checksum
		int i;
		for (i = 0; i<CKSUM_LEN; i++)
		{
			cksum_str[i] = packet[FLAG_LEN + i];
		}
		cksum_str[CKSUM_LEN] = '\0';
		/*
		while (cksum_str[0] == '0' && strlen(cksum_str) > 1) {
			++cksum_str;
		}
		*/
		int received_cksum = atoi(cksum_str);
		if (DEBUG_ALL || DEBUG_CKSUM)
			printf("Packet checksum: %d\n", received_cksum);

		// Peel off data from the message
		char data_filled[DATA_SIZE];
		memset(data_filled, 0, DATA_SIZE);
		for (i = 0; i<DATA_SIZE; i++)
		{
			data_filled[i] = packet[HEADER_LEN + i];
			if (i < data_size_bytes) {
				data[i] = packet[HEADER_LEN + i];
			}
		}

		printData(data_filled);

		int calculated_cksum = RSHash(data_filled, DATA_SIZE);

		if (DEBUG_ALL || DEBUG_CKSUM)
			printf("Checking calculated cksum (%d) against received cksum (%d)\n",
					calculated_cksum, received_cksum);

		// If checksums don't match, then corruption occurred.
		if( calculated_cksum != received_cksum)
		{
			// Data was corrupted during transmission
			printf("Data was corrupted during receive.  Trying %d more times...\n",
					(MAX_TRIES - num_receives - 1));
			int ack_status = NEG_ACK;
			int send_status = send(socket_num, &ack_status, sizeof(ack_status), ACK);
			if (send_status <= 0) {
				errno_info(errno);
			}
		} else {
			printf("Data successfully received!  Sending acknowledgement of success.\n");
			int ack_status = ACK_SUC;
			int send_status = send(socket_num, &ack_status, sizeof(ack_status), ACK);
			if (send_status <= 0)
				errno_info(errno);
			break;
		}
	} while (++num_receives < MAX_TRIES);
	if (num_receives < MAX_TRIES)
	{
		return SUCCESS;
	}
	else
	{
		printf("Too many failed attempts to receive data.  Giving up.\n");
		return SOCKERROR;
	}
}

int dkp_close(int socket_number) {
	//shutdown(socket_number, SHUT_RDWR);
	//errno_info(errno);
	/*
	printf("Waiting for socket %d to close", socket_number);
	int error_retval = close(socket_number);
	errno_info(errno);

	// Wait for the socket to close
	int rc;
	do {
		printf(".");
		rc = read(socket_number, NULL, 0);
	} while (rc >= 0);
	printf("\nSocket %d closed.\n", socket_number);
	return error_retval;
	*/
	return close(socket_number);
}

/*
 * RSHash was borrowed from the following website:
 * http://www.partow.net/programming/hashfunctions/#StringHashing
 *
 */
unsigned int RSHash(char* str, unsigned int len)
{
	unsigned int b    = 378551;
	unsigned int a    = 63689;
	unsigned int hash = 0;
	unsigned int i    = 0;
	char* str_orig    = str;

	for(i = 0; i < len; str++, i++)
	{
		hash = hash * a + (*str);
		a    = a * b;
	}

	if (DEBUG_ALL || DEBUG_CKSUM)	printf("RSHash(\"%s\", %d) = %d\n", str_orig, len, hash);
	return hash;
}

void printData(char data[]) {
	int i;
	printf("Data is: \"%s\" (%d bytes)\n", data, DATA_SIZE);
	printf("Data as bytes:\n");
	for (i=0; i<DATA_SIZE; i++) {
		if (i % 10 == 0) {
			printf("\n%4d - %4d:\t", i, (i+9));
		}
		printf("%4u ", data[i]);
	}
	printf("\n");
}

// Print information about a standard error.  See <errno.h> for more info.
void errno_info(int errsv) {
	switch (errsv) {
	case SUCCESS:
		break;
	case EHOSTDOWN:
		printf("ERROR %d: The computer at the specified IP seems to be turned off.\n", errsv);
		break;
	case ECONNREFUSED:
		printf("ERROR %d: Server process not running at specified IP\n", errsv);
		break;
	case EHOSTUNREACH:
		printf("ERROR %d: A firewall is probably blocking the connection\n", errsv);
		break;
	default:
		printf("ERROR %d: %s\n", errsv, strerror(errsv));
		break;
	}
}

void ack_status_info(int ack_status) {
	switch (ack_status) {
	case EMPTY_FIELD:
		printf("ERROR: ack_status remained unchanged after recv()-ing into it\n");
		break;
	case ACK_SUC:
		printf("Acknowledgment status: Successfully sent and acknowledged\n");
		break;
	case NEG_ACK:
		printf("Acknowledgment status: Package negatively acknowledged (corrupted)\n");
		break;
	default:
		printf("ERROR: Invalid value of ack_status: %d\n", ack_status);
		break;
	}
}

