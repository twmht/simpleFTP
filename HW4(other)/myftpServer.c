#include	"myftp.h"

#ifndef my_debug
#define my_debug
#endif

#ifdef my_debug
#define debug printf
#else
#define debug(arg,...)
#endif

// use ./myftpServer <port> <filename>
int main(int argc,char **argv)
{
	int socketfd;
	FILE *file;
	struct stat buf;
	struct sockaddr_in servaddr, clientaddr;
	char device[DEVICELEN];
	int port = atoi(argv[1]);
	char filename[FNAMELEN];

	/*int sockfd;
	int port = atoi(argv[1]);
	char device[] = "eth3";
	struct sockaddr_in servaddr, clientaddr;*/

	
	if(argc != 3)
	{
		printf("usage: ./myftpServer <port> <filename>\n");
		return 0;
	}
	
	if(lstat(argv[2], &buf) < 0) 
	{
		printf("unknow file : %s\n", argv[2]);
		return 0;
	}
	
	strcpy(filename, argv[2]);

	socketfd = socket(AF_INET,SOCK_DGRAM,0);
	
	if(getIFname(socketfd,device))
		errCTL("getIFname");
	debugf("network interface = %s\n",device);
	debugf("network port = %d\n",atoi(argv[1]));
	
	if(initServAddr(socketfd, atoi(argv[1]), device, &servaddr))
		errCTL("initServAddr");

    //Function: Server can serve multiple clients
    //Hint: Use loop, listenClient(), startMyFtpServer(), and ( fork() or thread ) 

	while (1) {
		if (listenClient(socketfd, port, filename, &clientaddr) == 1) {
			debug("enter strarMyftpServer()\n");
			clientaddr.sin_port = htons(port);
			startMyftpServer(&clientaddr, filename);
		}
		printf("2\n");
		debug("while!\n");
	}


	return 0;
}
