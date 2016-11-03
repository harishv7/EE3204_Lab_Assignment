/**********************************
tcp_ser.c: the source file of the server in tcp transmission 
***********************************/

#include "headsock.h"
#include <stdlib.h>

#define BACKLOG 10

// transmitting and receiving function
void str_ser(int sockfd, float error_prob);

int main(int argc, char **argv) {
	int sockfd, con_fd, ret;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;
	float error_prob;	// error probability (given by user)

	// process id
	pid_t pid;

	if (argc != 2) {
		printf("Given parameters do not match. Expected: <error_probability> from 0.00 to 100.00.\n");
	}

	// obtain error probability & convert from string to float
	error_prob = atof(argv[1]);

	// create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		printf("error in socket!");
		exit(1);
	}
		
	// assign socket family
	my_addr.sin_family = AF_INET;
	// assign port
	my_addr.sin_port = htons(MYTCP_PORT);
	// inet_addr("172.0.0.1");
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	bzero(&(my_addr.sin_zero), 8);

	// bind socket
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));

	if (ret < 0) {
		printf("error in binding");
		exit(1);
	}
	
	// listen on socket: returns 0 if successful, else -1
	ret = listen(sockfd, BACKLOG);

	if (ret < 0) {
		printf("error in listening");
		exit(1);
	}

	while (1) {
		printf("waiting for data\n");
		sin_size = sizeof (struct sockaddr_in);

		// accept the packet
		con_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (con_fd <0) {
			printf("error in accept\n");
			exit(1);
		}

		// create acception process
		if ((pid = fork()) == 0) {
			close(sockfd);
			// receive packet and response
			str_ser(con_fd, error_prob);
			close(con_fd);
			exit(0);
		} else {
			// parent process
			close(con_fd);
		}
	}

	close(sockfd);
	exit(0);
}

// method to receive packets from client, also sends ACK/NACK
void str_ser(int sockfd, float error_prob) {
	char buf[BUFSIZE];
	FILE *fp;
	struct pack_so recvs;
	struct ack_so ack;
	int end, n = 0;
	long lseek = 0;
	end = 0;
	float random_num;		// random number generated (to simulate damaged packet)
	int is_packet_damaged;	// flag to indicate if a packet is damaged or not
	int num_of_errors = 0;	// total number of errors so far
	int total_packets = 0;	// total number of packets received so far
	
	printf("Receiving data!\n");

	// initialise rand seed
	srand(time(NULL));

	// loop till you receive all packets
	while(!end) {
		// receive the packet
		if ((n = recv(sockfd, &recvs, PACKLEN, 0)) == -1) {
			printf("An error when receiving packet\n");
			exit(1);
		}

		// increment total packet count
		total_packets++;

		/** Determine if the packet is damaged (based on error probability) **/
		// if error probability is 0, this falls into the case of zero error
		// else, we consider the non-zero error case
		if(error_prob <= 0.0) {
			is_packet_damaged = 0;
		} else {
			// generate random number from 0 to 100
			random_num = ((float)rand() / (float)(RAND_MAX)) * 100;

			// check if random number is less than or equal to the error probability
			if(random_num <= error_prob) {
				is_packet_damaged = 1;
			} else {
				is_packet_damaged = 0;
			}
		}

		// if packet is not damaged, proceed normally and send ACK
		// else, send NACK
		if(is_packet_damaged == 0) {
			// if it is the end of the file (check last index if it is a null char)
			if (recvs.data[n - HEADLEN - 1] == '\0') {
				end = 1;
				n--;
			}

			// copy only the data
			memcpy((buf+lseek), recvs.data, n - HEADLEN);
			lseek += n - HEADLEN;

			// send ACK for each packet received
			ack.num = 1;
			ack.len = 0;

			// printf("Sending ACK for received packet\n");

			if ((n = send(sockfd, &ack, 2, 0)) == -1) {
				printf("Error while sending ACK packet!");
				exit(1);
			}
		} else {
			// send NACK for each packet received
			ack.num = -1;
			ack.len = 0;

			// printf("Sending NACK for received packet\n");

			// increment total number of errors
			num_of_errors++;

			if ((n = send(sockfd, &ack, 2, 0)) == -1) {
				printf("Error while sending NACK packet!");
				exit(1);
			}
		}
	}

	// open file to write received data in buffer
	if ((fp = fopen ("myTCPreceive.txt","wt")) == NULL) {
		printf("File doesn't exit\n");
		exit(0);
	}

	// write data into file
	fwrite (buf , 1 , lseek , fp);
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int) lseek);
	
	// print error metrics
	printf("Total number of errors received: %d\n", num_of_errors);
	printf("Total number of packets received: %d\n", total_packets);
	printf("Error Rate: %.5f%%\n", (num_of_errors / (float) total_packets * 100.0));
}