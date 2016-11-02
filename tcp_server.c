/**********************************
tcp_ser.c: the source file of the server in tcp transmission 
***********************************/

#include "headsock.h"

#define BACKLOG 10

// transmitting and receiving function
void str_ser(int sockfd);

int main(int argc, char **argv) {
	int sockfd, con_fd, ret;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;
	int error_prob;

	pid_t pid;

	if (argc != 2) {
		printf("Given parameters do not match. Expected: <error_probability>\n");
	}

	// obtain error probability & convert from string to int
	error_prob = atoi(argv[1]);

	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd <0) {
		printf("error in socket!");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYTCP_PORT);
	// inet_addr("172.0.0.1");
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	bzero(&(my_addr.sin_zero), 8);

	// bind socket
	ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));

	if (ret <0) {
		printf("error in binding");
		exit(1);
	}
	
	// listen
	ret = listen(sockfd, BACKLOG);

	if (ret <0) {
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
		if ((pid = fork())==0) {
			close(sockfd);
			// receive packet and response
			str_ser(con_fd);
			close(con_fd);
			exit(0);
		}
		else {
			// parent process
			close(con_fd);
		}
	}
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd) {
	char buf[BUFSIZE];
	FILE *fp;
	struct pack_so recvs;
	struct ack_so ack;
	int end, n = 0;
	long lseek=0;
	end = 0;
	
	printf("receiving data!\n");

	// loop till you receive all packets
	while(!end) {
		// receive the packet
		if ((n = recv(sockfd, &recvs, PACKLEN, 0)) == -1) {
			printf("error when receiving\n");
			exit(1);
		}

		// if it is the end of the file
		if (recvs.data[n - HEADLEN - 1] == '\0') {
			end = 1;
			n--;
		}

		// copy only the data
		memcpy((buf+lseek), recvs.data, n - HEADLEN);
		lseek += n - HEADLEN;

		// send ack/nack for each packet received
		ack.num = 1;
		ack.len = 0;

		printf("Sending ACK for received packet\n");

		if ((n = send(sockfd, &ack, 2, 0)) == -1) {
			printf("Error while sending ACK packet!");
			exit(1);
		}
	}

	if ((fp = fopen ("myTCPreceive.txt","wt")) == NULL) {
		printf("File doesn't exit\n");
		exit(0);
	}

	// write data into file
	fwrite (buf , 1 , lseek , fp);
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}