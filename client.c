#include "myftp.h"
int initCliAddr(int socketfd, int port, char *sendClient,struct sockaddr_in *addr)
{
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option

    int flag=1;
	if(setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST,&flag,sizeof(flag))<0){
		printf("setsockopt error!\n");
		exit(1);
	}
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
    /*addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);*/
    if(inet_aton(sendClient,&(addr->sin_addr)) == 0){
        perror("iner_aton");
        exit(1);
    }

    return 0;
}

int findServerAddr(int socketfd, char *filename,const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr)
{
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout
	int len=sizeof(struct sockaddr_in);
	struct bootServerInfo bootInfo;
    strcpy(bootInfo.filename,filename);

	if(sendto(socketfd,&bootInfo,sizeof(struct bootServerInfo),0,(struct sockaddr *)broadaddr,len)<0){
		printf("sendto error!\n");
		exit(1);
	}

    /*set time out*/
    struct timeval timeout = {3,0};
    if(setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval)) != 0){
        perror("setsockopt for time out");
        exit(1);
    }


    //receive from server
	bzero(servaddr,sizeof(struct sockaddr_in));
    int s = recvfrom(socketfd,&bootInfo,sizeof(bootInfo),MSG_WAITALL,(struct sockaddr*)&servaddr,&len);
    if(s == -1){
        printf("receive server response time out!!\n");
        exit(1);
    }
    else{
        printf("find myftpServer IP : %s\n",bootInfo.servAddr);
        printf("Myftp connent Port:%d\n",bootInfo.connectPort);
    }

    return 0;
}


int startMyftpClient(struct sockaddr_in *servaddr, const char *filename)
{
    //Function: Get file
    printf("file transmission start!!\n");

    /*fclose(file);*/
    printf("file transmission finish!!\n");
    return 0;
}

