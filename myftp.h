#ifndef __myftp_h__
#define __myftp_h__

#include	<stdio.h>
#include	<sys/socket.h>
#include	<string.h>
#include	<arpa/inet.h>
#include	<stdlib.h>
#include	<netdb.h>
#include 	<unistd.h>
#include	<netinet/in.h>
#include	<net/if.h>
#include	<linux/sockios.h>
#include	<time.h>
#include 	<errno.h>
#include	<signal.h>
#include	<sys/select.h>
#include 	<sys/stat.h>

#define	DEVICELEN	64
#define	HOSTNAME	64
#define	ADDRLEN		15
#define FNAMELEN	128
#define MAXLINE		1500
#define errCTL(x)   {perror(x); return -1;}

#define FRQ 		01		/* file request */
#define DATA    	02		/* data packet */
#define ACK 		03		/* acknowledgement */
#define ERROR   	04		/* error code */
#define	MFMAXDATA	512		/* data size */
#define MAXINTERFACES 20    /*max network interface"*/
#define FILE_NOT_EXIST 10
struct myFtphdr {
	short mf_opcode;
	unsigned short mf_cksum;
	union {
		unsigned short  mf_block;
		char mf_filename[1];
	}__attribute__ ((__packed__)) mf_u;
	char mf_data[1];
}__attribute__ ((__packed__));

#define mf_block	mf_u.mf_block
#define mf_filename	mf_u.mf_filename

struct bootServerInfo {
	char servAddr[ADDRLEN];
	int connectPort;
	char filename[FNAMELEN];
};

int getIFname(int socketfd, char *device);
int initServAddr(int socketfd, int port, const char *device,struct sockaddr_in *servaddr);
int initCliAddr(int socketfd, int port, char *sendClent,struct sockaddr_in *servaddr);
int findServerAddr(int socketfd, char *filename,const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr);
int listenClient(int socketfd, int port, char *filename, struct sockaddr_in *clientaddr);
int startMyftpServer(struct sockaddr_in *clientaddr, const char *filename);
int startMyftpClient(struct sockaddr_in *servaddr, const char *filename);
int send_packet(int socketfd,struct myFtphdr * packet,struct sockaddr_in *addr,unsigned short block,short opcode,int size);
unsigned short in_cksum(unsigned short *addr, int len);

#ifdef DEBUG
#define debugf(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define debugf(fmt, args...)
#endif


#endif
