#include	"myftp.h"

int getIFname(int socketfd,char *device)
{
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list 

	return 0;
}

int initServAddr(int socketfd, int port, const char *device,struct sockaddr_in *addr)
{
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd 
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)

	return 0;
}

int initCliAddr(int socketfd, int port, char *sendClient,struct sockaddr_in *addr)
{
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option
	
	return 0;
}

int findServerAddr(int socketfd, char *filename,const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr)
{
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout

	return 0;
}

int listenClient(int socketfd, int port, char *filename, struct sockaddr_in *clientaddr)
{
    //Function: Wait for broadcast message from client
    //          As receive broadcast message, check file exist or not
    //          Set bootServerInfo with server address and new port, and send back to client

	return 0;
}

int startMyftpServer(struct sockaddr_in *clientaddr, const char *filename)
{
    //Function: Send file
	printf("file transmission start\n");

	printf("send file : <%s> to %s \n", filename, inet_ntoa(clientaddr->sin_addr));
	printf("%lu bytes sent\n", index);
	printf("file transmission finish!!\n");
	return 0;
}

int startMyftpClient(struct sockaddr_in *servaddr, const char *filename)
{
    //Function: Get file
	printf("file transmission start!!\n");

	fclose(file);
	printf("file transmission finish!!\n");
	return 0;
}

static unsigned short in_cksum(unsigned short *addr, int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	
	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}
