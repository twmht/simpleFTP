#ifndef __myftp_h__
#define __myftp_h__
#include 	<arpa/inet.h>
#include 	<errno.h>
#include 	<fcntl.h>
#include	<linux/sockios.h>
#include	<net/if.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include 	<pthread.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/select.h>#include	<sys/socket.h>
#include 	<sys/stat.h>
#include 	<sys/types.h>
#include 	<sys/utsname.h>
#include	<time.h>
#include 	<unistd.h>

#define MAXFILES 2
#define FILES1 "dict001"
#define FILES2 "testfile"
#define MAXSOCKFD	1000#define DEVICE 		"eth0"
#define	DEVICELEN	64
#define	HOSTNAME	64
#define	ADDRLEN		20
#define FNAMELEN	128
#define MAXLINE		1500/* For Opcode */
/*different*/
#define REQ 		01		/* Client Request */
#define RES		02		/* Server Response */
#define DAT 	   	03		/* Data Package */
#define ACK 		04		/* Acknowledgement */
#define ERR	   	05		/* Error Message */
#define FIN		06		/* Finish Message */
/* For Ans */
#define NOK		00		/* Answer not OK */
#define OK		01		/* Answer OK */
/* For Data */
#define	MFMAXDATA	512		/* Data Size */
struct myFtphdr{
	short mf_opcode;
	unsigned short mf_cksum;
	union{
		short mf_ans;
		unsigned short mf_block;
		char mf_filename[1];
	}__attribute__ ((__packed__)) mf_u;
	char mf_data[1];
}__attribute__ ((__packed__));
#define mf_block	mf_u.mf_block
#define mf_ans		mf_u.mf_ans
#define mf_filename	mf_u.mf_filename
struct bootServerInfo{
	char servAddr[ADDRLEN];
	int connectPort;
};

#endif
