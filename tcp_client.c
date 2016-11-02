/*******************************
tcp_client.c: the source file of the client in tcp transmission 
********************************/

#include "headsock.h"

#define FILENAME "myfile.txt"

// transmission function
float str_cli(FILE *fp, int sockfd, long *len);

// calculate the time interval between out and in
void tv_sub(struct  timeval *out, struct timeval *in);	    

int main(int argc, char **argv) {
	int sockfd, ret;
	float ti, rt;
	long len;
	struct sockaddr_in ser_addr;
	char ** pptr;
	struct hostent *sh;
	struct in_addr **addrs;
	FILE *fp;

	if (argc != 2) {
		printf("Parameters do not match. Expected: <server_name/port>\n");
	}

	// get host's information
	sh = gethostbyname(argv[1]);
	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}

	// print the remote host's information
	printf("canonical name: %s\n", sh->h_name);

	for (pptr=sh->h_aliases; *pptr != NULL; pptr++) {
		printf("the aliases name is: %s\n", *pptr);
	}

	switch(sh->h_addrtype) {
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
        
	addrs = (struct in_addr **)sh->h_addr_list;

	// create the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd <0) {
		printf("error in socket");
		exit(1);
	}

	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYTCP_PORT);

	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));

	bzero(&(ser_addr.sin_zero), 8);

	// connect the socket with the host
	ret = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));

	if (ret != 0) {
		printf ("connection failed\n"); 
		close(sockfd); 
		exit(1);
	}
	
	if((fp = fopen ("myfile.txt","r+t")) == NULL) {
		printf("File doesn't exit\n");
		exit(0);
	}

	// perform the transmission and receiving
	ti = str_cli(fp, sockfd, &len);

	//caculate the average transmission rate
	rt = (len/(float)ti);

	printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\n", ti, (int)len, rt);

	close(sockfd);
	fclose(fp);

	exit(0);
}

float str_cli(FILE *fp, int sockfd, long *len) {
	char *buf;
	long lsize, ci;
	struct ack_so ack;
	struct pack_so sends;
	int n, slen;
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	ci = 0;

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	printf("The file length is %d bytes\n", (int) lsize);
	printf("the packet length is %d bytes\n", DATALEN);

	// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  	// copy the file into the buffer.
	fread (buf,1,lsize,fp);

  	/*** the whole file is loaded in the buffer. ***/
  	// append the end byte (null byte to terminate string)
	buf[lsize] ='\0';

	// get the current time
	gettimeofday(&sendt, NULL);

	while(ci<= lsize) {
		if ((lsize+1-ci) <= DATALEN) {
			slen = lsize+1-ci;
		} else {
			slen = DATALEN;
		}

		memcpy(sends.data, (buf+ci), slen);

		// assign packet properties (len = length being sent, num = 0)
		sends.len = slen;
		sends.num = 0;

		// send the data
		n = send(sockfd, &sends, (sends.len + HEADLEN), 0);

		if(n == -1) {
			printf("send error!");
			exit(1);
		}

		// wait for ack/nack from receiver
		if ((n = recv(sockfd, &ack, 2, 0)) == -1) {
			printf("Error when receiving ACK/NACK\n");
			exit(1);
		}

		if(ack.num == 1) {
			printf("Received ACK from Server\n");
			ci += slen;
		} else {
			printf("Received NACK from Server\n");
			// continue without incrementing ci
			continue;
		}
	}

	// get current time
	gettimeofday(&recvt, NULL);

	*len= ci;

	// get the whole trans time
	tv_sub(&recvt, &sendt);

	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;

	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in) {
	if ((out->tv_usec -= in->tv_usec) <0) {
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}

	out->tv_sec -= in->tv_sec;
}