#include	"myftp.h"

// use ./myftpServer <port> <filename>
int main(int argc,char **argv)
{
    int socketfd;
    FILE *file;
    struct stat buf;
    struct sockaddr_in servaddr, clientaddr;
    char device[DEVICELEN];

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

    socketfd = socket(AF_INET,SOCK_DGRAM,0);

    if(getIFname(socketfd,device))
        errCTL("getIFname");
    debugf("network interface = %s\n",device);
    debugf("network port = %d\n",atoi(argv[1]));

    if(initServAddr(socketfd, atoi(argv[1]), device, &servaddr))
        errCTL("initServAddr");

    //Function: Server can serve multiple clients
    //Hint: Use loop, listenClient(), startMyFtpServer(), and ( fork() or thread ) 
    while(1){
        if (listenClient(socketfd, port, filename, &clientaddr) == 1) {
            printf("enter strarMyftpServer()\n");
            clientaddr.sin_port = htons(port);
            /*startMyftpServer(&clientaddr, filename);*/
        }
    }

    return 0;
}
