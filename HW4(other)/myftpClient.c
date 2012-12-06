#include	"myftp.h"


#ifndef my_debug
#define my_debug
#endif

#ifdef my_debug
#define debug printf
#else
#define debug(arg,...)
#endif

// use ./myftpClient <port> <filename>
int main(int argc, char **argv)
{

	/*int socketfd;
	int port;
	char filename[FNAMELEN];
	char sendClent[] = "eth3";
	struct sockaddr_in broadaddr, clientaddr, servaddr;
	
	if(argc != 3)
	{
		printf("usage: ./myftpClient <port> <filename>\n");
		return 0;
	}

	strcpy(filename, argv[2]);
	debug("filename: %s\n", argv[2]);

	port = atoi(argv[1]);
	debug("port: %d\n", port);

	
	socketfd = socket(AF_INET,SOCK_DGRAM,0);
	if(initCliAddr(socketfd, atoi(argv[1]), "255.255.255.255", &broadaddr))
		errCTL("initCliAddr");
		
	if(findServerAddr(socketfd, argv[2], &broadaddr, &servaddr))
		errCTL("findServerAddr");
	
	if(startMyftpClient(&servaddr, argv[2]))
			errCTL("startMyftpClient");
	
	return 0;*/




int socketfd;
	int port;
	char filename[FNAMELEN];
	char sendClent[64]; //= "eth3";
	struct sockaddr_in broadaddr, clientaddr, servaddr;

	if (argc != 3) {
		printf("usage: ./myftpClient <port> <filename>\n");
		return -1;
	}

	strcpy(filename, argv[2]);
	debug("filename: %s\n", argv[2]);

	port = atoi(argv[1]);
	debug("port: %d\n", port);

	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket() error!\n");
		return -1;
	}
///////////////////////////////////


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

    //*sendClent= ifreqs[1].ifr_name;
sprintf(sendClent,"%s",ifreqs[1].ifr_name);
    //devicenum = ifreqs[1].ifr_name;
    //printf("network interface = %s\n",device);


//////////////////////////////////
	initCliAddr(socketfd, port, sendClent, &clientaddr);

	memset(&broadaddr, 0, sizeof(struct sockaddr));
	broadaddr.sin_family = AF_INET;
	broadaddr.sin_port = htons(port);
	broadaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	//broadaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//broadaddr.sin_addr.s_addr = inet_addr("140.117.168.103");

	int servans;

	if ((servans = findServerAddr(socketfd, filename, &broadaddr, &servaddr))
			< 0) {
		printf("findServerAddr() error!\n");
		return -1;
	}
	if (servans == 0) {
		return 0;
	}

	servaddr.sin_port = htons(port);

	if (startMyftpClient(&servaddr, filename) < 0) {
		printf("startMyftpClient() error!\n");
		return -1;
	}

	printf("file transmission finished!!\n");

	debug("findServerAddr() ok\n");

	return 0;
}
