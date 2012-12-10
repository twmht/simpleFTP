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
    printf("Myftp Server Start!!\nWait clients\n");
    printf("Share file: %s\n",argv[2]);
    int port = atoi(argv[1]);
    char filename[FNAMELEN];
    srand(time(NULL));
    int listen_result;
    while(1){
        if ((listen_result = listenClient(socketfd, port, filename, &clientaddr))>0) {
            //send back
            struct bootServerInfo bootInfo;
            bzero(&bootInfo,sizeof(struct bootServerInfo));
            char *client_ip = inet_ntoa(clientaddr.sin_addr);
            printf("Client from %s connect!!\n",client_ip);
            port = port + (rand()%1000);
            bootInfo.connectPort = port;
            if (listen_result != FILE_NOT_EXIST){
                strcpy(bootInfo.filename,filename);
            }
            else{
                bootInfo.filename[0] = '\0';
            }
            strcpy(bootInfo.servAddr,inet_ntoa(servaddr.sin_addr));
            printf("Myftp connect port : %d\n",bootInfo.connectPort);

            if((sendto(socketfd, &bootInfo, sizeof(bootInfo), 0, (struct sockaddr *)&clientaddr, sizeof(struct sockaddr_in)))<0){
                perror("sendto error");
                exit(1);
            }
            printf("enter strarMyftpServer()\n");
            /*clientaddr.sin_port = htons(port);*/
            /*startMyftpServer(&clientaddr, filename);*/
        }
    }

    return 0;
}
