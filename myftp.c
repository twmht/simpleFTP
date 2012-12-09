#include "myftp.h"
int getIFname(int socketfd,char *device)
{
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list 
    int sock;
    struct ifconf ifconf;
    struct ifreq ifreq[MAXINTERFACES];
    int interfaces;
    int i;

    // Point ifconf's ifc_buf to our array of interface ifreqs.
    ifconf.ifc_buf = (char *) ifreq;

    // Set ifconf's ifc_len to the length of our array of interface ifreqs.
    ifconf.ifc_len = sizeof(ifreq);

    //  Populate ifconf.ifc_buf (ifreq) with a list of interface names and addresses.
    if (ioctl(socketfd, SIOCGIFCONF, &ifconf) == -1){
        perror("socket");
        exit(1);
    }

    // Divide the length of the interface list by the size of each entry.
    // This gives us the number of interfaces on the system.
    interfaces = ifconf.ifc_len / sizeof(ifreq[0]);
    int use;
    for (i = 0; i < interfaces; i++) {
        if(strcmp(ifreq[i].ifr_name,"lo") != 0){
            strcpy(device,ifreq[i].ifr_name);
        }
    }
    printf("The number of interface is %d ,Using %s as network interface\n",interfaces,device);

    return 0;
}

int initServAddr(int socketfd, int port, const char *device,struct sockaddr_in *addr)
{
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd 
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)
    if (setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE,device, sizeof(device)) < 0) {
        printf("setsockopt() SO_BINDTODEVICE error!\n");
        perror("setsockopt");
        exit(1);
    }
    bzero(addr,sizeof(*addr));
    addr->sin_family=AF_INET;
    addr->sin_port= htons(port);
    addr->sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(socketfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0){
		printf("Bind error!\n");
		exit(1);
	}
    
    struct ifreq interface;
	bzero(&interface,sizeof(struct ifreq));
	strcpy(interface.ifr_name,device);
	if(ioctl(socketfd,SIOCGIFADDR,&interface)<0){
		printf("ioctl error!\n");
		exit(1);
	}

	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(struct sockaddr_in));
	serveraddr = *((struct sockaddr_in*)&interface.ifr_addr);
	printf("Server IP : %s\n", inet_ntoa(serveraddr.sin_addr));

    return 0;
}

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

int listenClient(int socketfd, int port, char *filename, struct sockaddr_in *clientaddr)
{
    //Function: Wait for broadcast message from client
    //          As receive broadcast message, check file exist or not
    //          Set bootServerInfo with server address and new port, and send back to clientint len = sizeof(struct sockaddr_in);
    int len = sizeof(struct sockaddr_in);
	struct bootServerInfo bootInfo;
    bzero(&bootInfo,sizeof(struct bootServerInfo));
	if(recvfrom(socketfd, &bootInfo, sizeof(bootInfo), 0, (struct sockaddr *)clientaddr, &len) < 0){
		perror("recvfrom error");
		exit(1);
	}

    return 0;
}

int startMyftpServer(struct sockaddr_in *clientaddr, const char *filename)
{
    //Function: Send file
    printf("file transmission start\n");

    printf("send file : <%s> to %s \n", filename, inet_ntoa(clientaddr->sin_addr));
    /*printf("%lu bytes sent\n", index);*/
    printf("file transmission finish!!\n");
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
