#include <string.h>

#include "myftp.h"


#ifndef my_debug
#define my_debug
#endif

#ifdef my_debug
#define debug printf
#else
#define debug(arg,...)
#endif


int getIFname(int socketfd,char *device)
{
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list 

	int sock, rval;
    static struct ifreq ifreqs[20];
    struct ifconf ifconf;
    int  nifaces, i;
    sock = socket(AF_INET,SOCK_STREAM,0);

    memset(&ifconf,0,sizeof(ifconf));
    ifconf.ifc_buf = (char*) (ifreqs);
    ifconf.ifc_len = sizeof(ifreqs);

    if(rval = ioctl(sock, SIOCGIFCONF , (char*) &ifconf ) < 0) return -1;
    close(sock);
    nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

    device = ifreqs[1].ifr_name;
    //devicenum = ifreqs[1].ifr_name;
    printf("network interface = %s\n",device);

///

if (setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, device,
			sizeof(device)) < 0) {
		printf("setsockopt() SO_BINDTODEVICE error!\n");
		perror("setsockopt");
		return -1;
	}

	return 0;
}

int initServAddr(int socketfd, int port, const char *device,struct sockaddr_in *addr)
{
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd 
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)

	printf("MyFtp Server start!\n");

	/*if (setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, device,
			sizeof(device)) < 0) {
		printf("setsockopt() SO_BINDTODEVICE error!\n");
		perror("setsockopt");
		return -1;
	}*/

	int sock_opt = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void*) &sock_opt,
			sizeof(sock_opt)) < 0) {
		printf("startMyftpServer setsockopt() error!\n");
		perror("setsockopt");
		return -1;
	}

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = htons(INADDR_ANY);

	if (bind(socketfd, (struct sockaddr *) addr, sizeof(struct sockaddr))
			< 0) {
		printf("bind() error!\n");
		perror("bind");
		return -1;
	}

	return 0;


}

int initCliAddr(int socketfd, int port, char *sendClient,struct sockaddr_in *addr)
{
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option


int optval = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int))
			< 0) {
		printf("setsockopt() broadcast error!\n");
		perror("setsockopt");
		return -1;
	}

	//timeout = 3 sec;
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			sizeof(struct timeval)) < 0) {
		printf("setsockopt() timeout error!\n");
		perror("setsockopt");
		return -1;
	}

	debug("receive port: %d\n", ntohs(addr->sin_port));


	
	return 0;
}

int findServerAddr(int socketfd, char *filename,const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr)
{
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout

int addrlen = sizeof(struct sockaddr);
	char buffer[FNAMELEN];

	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, filename);

	if (sendto(socketfd, buffer, FNAMELEN, 0,
			(const struct sockaddr *) broadaddr, sizeof(struct sockaddr)) < 0) {
		printf("sendto() error!\n");
		perror("sendto");
		return -1;
	}

	int buf_len;

	clock_t endwait = clock() + 3 * CLOCKS_PER_SEC;
	while (clock() < endwait) {
		memset(buffer, 0, sizeof(buffer));
		if ((buf_len = recvfrom(socketfd, buffer, sizeof(buffer), 0,
				(struct sockaddr *) servaddr, &addrlen)) < 0) {
			printf("no server answer!\n");
			return 0;
		} else {
			struct in_addr temp;
			inet_aton(buffer, &temp);
			if ((temp.s_addr) == (servaddr->sin_addr.s_addr)) {
				return 1;
			} else {
				debug("buffer address %x", temp.s_addr);
				debug("server address %x", servaddr->sin_addr.s_addr);
				printf("ip address different\n");
				return 1;
			}
			//check the packet
		}
	}

	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("startMyftpServer socket() error!\n");
		return -1;
	}



	return 0;
}

int listenClient(int socketfd, int port, char *filename, struct sockaddr_in *clientaddr)
{
    //Function: Wait for broadcast message from client
    //          As receive broadcast message, check file exist or not
    //          Set bootServerInfo with server address and new port, and send back to client

printf("share file : %s\n", filename);
	printf("wait clinet!\n");

	int addr_len = sizeof(struct sockaddr);
	char buffer[256];

	memset(buffer, 0, sizeof(buffer));

	memset(clientaddr, 0, sizeof(struct sockaddr));
	clientaddr->sin_family = AF_INET; /* host byte order */
	clientaddr->sin_port = htons(port);
	clientaddr->sin_addr.s_addr = htons(INADDR_ANY);

	if (recvfrom(socketfd, buffer, sizeof(buffer), 0,
			(struct sockaddr *) clientaddr, &addr_len) < 0) {
		printf("listenClient recvfrom() error!\n");
		perror("recvfrom");
		return -1;
	} else {
		printf("test ok!\n");
		if ((strcmp(buffer, filename) == 0)) {
			memset(buffer, 0, sizeof(buffer));
			struct sockaddr_in temp;
			getsockname(socketfd, (struct sockaddr *) &temp, &addr_len);
			strcpy(buffer, inet_ntoa(temp.sin_addr));
			debug("server address1: %s\n", buffer);
			if (sendto(socketfd, buffer, sizeof(buffer), 0,
					(struct sockaddr *) clientaddr, addr_len) < 0) {
				printf("listenClient sendto() error!\n");
				perror("sendto");
				return -1;
			}
			printf("1\n");
			return 1;
		}
	}
	debug("listenClient received\n");




	return 0;
}

int startMyftpServer(struct sockaddr_in *clientaddr, const char *filename)
{
    /*//Function: Send file
	printf("file transmission start\n");

	printf("send file : <%s> to %s \n", filename, inet_ntoa(clientaddr->sin_addr));
	printf("%lu bytes sent\n", index);
	printf("file transmission finish!!\n");
	return 0;*/
/////
	int socketfd;
	struct sockaddr_in servaddr;

	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket() error!\n");
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			sizeof(struct timeval)) < 0) {
		printf("setsockopt() timeout error!\n");
		perror("setsockopt");
		return -1;
	}

	int sock_opt = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void*) &sock_opt,
			sizeof(sock_opt)) < 0) {
		printf("startMyftpServer setsockopt() error!\n");
		perror("setsockopt");
		return -1;
	}

	memset(&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(ntohs(clientaddr->sin_port) + 100);
	debug("server rec port: %d\n", ntohs(servaddr.sin_port));

	if (bind(socketfd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr))
			< 0) {
		printf("bind() error!\n");
		perror("bind");
		return -1;
	}

	int p_len = sizeof(struct sockaddr);
	char buffer[MFMAXDATA + 4];
	struct myFtphdr *hdr = (struct myFtphdr *) buffer;
	memset(buffer, 0, sizeof(buffer));

	char recvbuf[MFMAXDATA + 4];
	struct myFtphdr *rechdr = (struct myFtphdr *) recvbuf;

	FILE *fp;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("file not found!\n");
		return -1;
	}

	clock_t endwait = clock() + 30 * CLOCKS_PER_SEC;

	clientaddr->sin_port = servaddr.sin_port;
	debug("clientaddr address: %x\n", clientaddr->sin_addr.s_addr);

	while (1) {
		memset(recvbuf, 0, sizeof(recvbuf));
		if (recvfrom(socketfd, recvbuf, sizeof(recvbuf), 0,
				(struct sockaddr *) clientaddr, &p_len) < 0) {
			printf("myftp connect port : %d\n", ntohs(clientaddr->sin_port));
			printf("starMyftpServer listenClient recvfrom() error!\n");
			perror("recvfrom");
			if (endwait > clock()) {
				//time out retransmission
				if ((hdr->mf_block != 0) && (sendto(socketfd, buffer, FNAMELEN
						+ 4, 0, (const struct sockaddr *) clientaddr,
						sizeof(struct sockaddr)) < 0)) {
					printf("sendto() error!\n");
					perror("sendto");
					return -1;
				}
			} else {
				//time out
				printf("time out file transfer fail!\n");
				return -1;
			}
		} else {
			if (checksum_verify((struct myFtphdr *) recvbuf, sizeof(p_len))) {
				endwait = clock() + 30 * CLOCKS_PER_SEC;
				switch (rechdr->mf_opcode) {

				case FRQ:
					hdr->mf_opcode = DATA;
					hdr->mf_block = 1;
					readfile(hdr->mf_data, fp);
					if (sendto(socketfd, buffer, FNAMELEN + 4, 0,
							(const struct sockaddr *) clientaddr,
							sizeof(struct sockaddr)) < 0) {
						printf("sendto() error!\n");
						perror("sendto");
						return -1;
					}
					break;

				case ACK:
					if (rechdr->mf_block == hdr->mf_block) {
						++(hdr->mf_block);
						if (!feof(fp)) {
							readfile(hdr->mf_data, fp);
							if (sendto(socketfd, buffer, FNAMELEN + 4, 0,
									(const struct sockaddr *) clientaddr,
									sizeof(struct sockaddr)) < 0) {
								printf("sendto() error!\n");
								perror("sendto");
								return -1;
							}
						} else {
							int size = ftell(fp);
							printf("%d bytes send\n", size);
							fclose(fp);
							return 0;
						}
					}
					break;

				case ERROR:
					if (rechdr->mf_block == hdr->mf_block) {
						if (sendto(socketfd, buffer, FNAMELEN + 4, 0,
								(const struct sockaddr *) clientaddr,
								sizeof(struct sockaddr)) < 0) {
							printf("sendto() error!\n");
							perror("sendto");
							return -1;
						}
					}
					break;

				default:
					break;
				}
			}
		}
	}

	return 0;


}

int startMyftpClient(struct sockaddr_in *servaddr, const char *filename)
{
    //Function: Get file
/*	printf("file transmission start!!\n");

	fclose(file);
	printf("file transmission finish!!\n");
	return 0;*/

int socketfd;
	char buffer[FNAMELEN + 4];
	char recvbuf[MFMAXDATA + 4];
	struct sockaddr_in clientaddr;

	struct myFtphdr *hdr = (struct myFtphdr *) buffer;
	struct myFtphdr *rechdr = (struct myFtphdr *) recvbuf;

	int addrlen;

	int block = 0;

	FILE *fp;

	char newname[FNAMELEN] = "client_";

	strcat(newname, filename);

	fp = fopen(newname, "w");

	srand(time(NULL));

	memset(&clientaddr, 0, sizeof(struct sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(ntohs(servaddr->sin_port) + 100);
	clientaddr.sin_addr.s_addr = htons(INADDR_ANY);

	//servaddr->sin_port = clientaddr.sin_port;

	debug("file transfer port %d\n", ntohs(clientaddr.sin_port));

	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("startMyftpClient socket() error!\n");
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			sizeof(struct timeval)) < 0) {
		printf("setsockopt() timeout error!\n");
		perror("setsockopt");
		return -1;
	}

	if (bind(socketfd, (struct sockaddr *) &clientaddr, sizeof(struct sockaddr))
			< 0) {
		printf("startMyftpClient bind() error!\n");
		perror("bind");
		return -1;
	}

	memset(buffer, 0, sizeof(buffer));
	hdr->mf_opcode = FRQ;
	strcpy(hdr->mf_filename, filename);
	checksum_calculate(hdr, FNAMELEN + 4);

	memset(recvbuf, 0, sizeof(recvbuf));

	servaddr->sin_port = clientaddr.sin_port;

	if (sendto(socketfd, buffer, FNAMELEN + 4, 0,
			(const struct sockaddr *) servaddr, sizeof(struct sockaddr)) < 0) {
		printf("sendto() error!\n");
		perror("sendto");
		return -1;
	}

	debug("dest port: %d\n", ntohs(servaddr->sin_port));

	clock_t endwait = clock() + 30 * CLOCKS_PER_SEC;

	while (1) {
		if ((recvfrom(socketfd, recvbuf, sizeof(recvbuf), 0,
				(struct sockaddr *) servaddr, &addrlen)) < 0) {
			printf("no server answer!\n");
			if (endwait > clock()) {
				if (sendto(socketfd, buffer, FNAMELEN + 4, 0,
						(const struct sockaddr *) servaddr,
						sizeof(struct sockaddr)) < 0) {
					printf("retransmission sendto() error!\n");
					perror("sendto");
				}
			} else {
				printf("time out!\n");
				return -1;
			}
		} else {
			endwait = clock() + 30 * CLOCKS_PER_SEC;
			if (checksum_verify(rechdr, MFMAXDATA + 4)) {
				switch (rechdr->mf_opcode) {

				case DATA:
					if (rechdr->mf_block == (block + 1)) {
						memset(buffer, 0, sizeof(buffer));
						hdr->mf_opcode = ACK;
						++block;
						hdr->mf_block = block;
						checksum_calculate(hdr, FNAMELEN + 4);
						if (sendto(socketfd, buffer, FNAMELEN + 4, 0,
								(const struct sockaddr *) servaddr,
								sizeof(struct sockaddr)) < 0) {
							printf("ACK sendto() error!\n");
							perror("sendto");
							return -1;
						}
						fwrite(rechdr->mf_data, 1, MFMAXDATA, fp);
						if (rechdr->mf_data[511] == '\0') {
							int size = ftell(fp);
							printf("%d bytes received\n", size);
							return 0;
						}
					}
					break;

				default:
					break;
				}
			} else {
				//checksum error
			}
		}
	}

	return -1;



}

static unsigned short in_cksum(unsigned short *addr, int len)
{

}


int readfile(char *data, FILE *fp) {
	fread(data, 1, MFMAXDATA, fp);
	return 0;
}

int checksum_calculate(struct myFtphdr *packet, int p_len) {
	unsigned int checksum = 0;

	int i = 0;
	while (i < p_len) {
		checksum += (*((char *) packet + i) << 8)
				+ (*((char *) packet + i + 1));
		i += 2;
	}
	checksum = (checksum & 0xffff) + (checksum >> 16);

	packet->mf_cksum = ~checksum;
	debug("checksum %.4x\n", packet->mf_cksum);
	return 0;
}
///////////////////////////////////////////////////////////////////
int checksum_verify(const struct myFtphdr *packet, int p_len) {
	unsigned int checksum = 0;

	int i = 0;
	while (i < p_len) {
		checksum += (*((char *) packet + i) << 8)
				+ (*((char *) packet + i + 1));
		i += 2;
	}
	checksum = (checksum & 0xffff) + (checksum >> 16);

	if (((~checksum) == packet->mf_cksum)) {
		return 1;
	} else {
		return 0;
	}
}
