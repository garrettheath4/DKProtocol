/*
 * dkp.h
 *
 *  Created on: Feb 25, 2012
 *      Author: davisl
 */

#ifndef DKP_H_
#define DKP_H_

//
// Constants
//

// Port number that this protocol will use to talk over
#define PORT_NUM 2049	//65000

// Error codes
#define SUCCESS     0
#define NODENOTAVL -1
#define SOCKERROR  -2

// Handshaking Flags
#define NORM     0
#define SYN      1
#define SYN_ACK  2
#define FIN_ACK  3

//Acknowledgment for sliding window
#define ACK 4

//Acknowledgment statuses
#define ACK_SUC 5
#define NEG_ACK 6

// Max characters allowed in the data field of the message
#define DATA_SIZE 1024

//
// FUNCTION HEADERS
//

/* Function: dkp_connect
 * Input:  char ip_addr[] - The IP address as a string in the format "123.456.789.123"
 * Output: int socket_num - The identifier of the socket that was created for this connection
 */
int dkp_connect(char ip_addr[]);

/* Function: dkp_accept
 * Input:  n/a
 * Output: int socket_num - The identifier of the socket that was created for this connection
 */
int dkp_accept();

/* Function dkp_send
 * Input:  int socket_num - The identifier of the desired socket returned from dkp_connect()
 *         string data - The message to send as a string of text
 *         unsigned long int data_size_bytes - The size of the sending buffer 'data'
 * Output: int error_number - The return status of the dkp_send.  A value of 0 means no error.
 */
int dkp_send(int socket_num, char data[], unsigned long int data_size_bytes);

/* Function dkp_receive
 * Input:  int socket_number - The identifier of the desired socket returned from dkp_connect()
 *         string data - The message received from the sender over the given connection.
 *         unsigned long int data_size_bytes - The size of the receiving buffer 'data'
 * Output: int error_number - The return status of the dkp_send.  A value of 0 means no error.
 */
int dkp_recv(int socket_number, char data[], unsigned long int data_size_bytes);

/* Function dkp_close
 * Input:  int socket_number - The identifier of the desired socket returned from dkp_connect()
 * Output: 0 if dkp_close was successful or -1 if dkp_close failed (as
 *         specified by close(2))
 */
int dkp_close(int socket_number);

#endif /* DKP_H_ */
