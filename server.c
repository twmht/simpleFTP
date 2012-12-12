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
    struct sockaddr_in any;
    bzero(&any,sizeof(any));
    any.sin_family=AF_INET;
    any.sin_port= htons(port);
    any.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(socketfd, (struct sockaddr *)(&any), sizeof(struct sockaddr_in)) < 0){
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

    /*addr = ((struct sockaddr_in*)&interface.ifr_addr);*/
    memcpy(addr,(struct sockaddr_in*)&interface.ifr_addr,sizeof(interface.ifr_addr));
    printf("Server IP : %s\n", inet_ntoa(addr->sin_addr));

    return 0;
}
int startMyftpServer(struct sockaddr_in *clientaddr, const char *filename)
{
    int socketfd;
    if((socketfd= socket(AF_INET,SOCK_DGRAM,0))<0){
        perror("Socket Error!");
        exit(1);
    }
    /*set time out*/
    struct timeval timeout = {3,0};
    if(setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval)) != 0){
        perror("setsockopt for time out");
        exit(1);
    }
    //Function: Send file
    struct myFtphdr *FRQ_packet;
    int f_len = strlen(filename)+1;
    int FRQ_size = f_len+4;
    FRQ_packet = (struct myFtphdr *)malloc(FRQ_size);
    //error and ack packet
    struct myFtphdr *ACK_ERROR_packet;
    int ACK_ERROR_size = 6;
    ACK_ERROR_packet = (struct myFtphdr *)malloc(ACK_ERROR_size);

    int sockaddr_len = sizeof(struct sockaddr_in);
    while(1){
        //recvive FRQ
        if((recvfrom(socketfd,FRQ_packet,FRQ_size,MSG_WAITALL,(struct sockaddr*)clientaddr,&sockaddr_len))<0){
            /*errCTL("recvfrom FRQ");*/
            printf("time out waiting FRQ\n,request client to resend\n");
            //send ERROR packet for FRQ with block number = 0
            send_packet(socketfd,ACK_ERROR_packet,clientaddr,0,ERROR,ACK_ERROR_size);
        }
        else if(in_cksum((unsigned short *)FRQ_packet,FRQ_size)!=0){
            send_packet(socketfd,ACK_ERROR_packet,clientaddr,0,ERROR,ACK_ERROR_size);
        }
    }
    printf("receive FRQ packet\n");
    struct myFtphdr *data_packet;
    int data_packet_size = MFMAXDATA+6;
    data_packet = (struct myFtphdr *)malloc(data_packet_size);
    //initial block
    int block = 1;
    FILE *fin = fopen(filename,"rb");
    printf("send file : <%s> to %s \n", filename, inet_ntoa(clientaddr->sin_addr));
    while(1){
        printf("file transmission start\n");
        fread(data_packet->mf_data,1,MFMAXDATA,fin);
        if(send_packet(socketfd,data_packet,clientaddr,block,DATA,data_packet_size) == -1){
            exit(1);
        }
        //wait ACK packet
        if((recvfrom(socketfd,ACK_ERROR_packet,ACK_ERROR_size,MSG_WAITALL,(struct sockaddr*)clientaddr,&sockaddr_len))<0){
            /*errCTL("recvfrom FRQ");*/
            printf("time out waiting ACK\n,request client to resend\n");
            send_packet(socketfd,ACK_ERROR_packet,clientaddr,block,ERROR,ACK_ERROR_size);
            continue;
        }
        else if(in_cksum((unsigned short *)ACK_ERROR_packet,ACK_ERROR_size)!=0){
            //check sum error,resend again
            send_packet(socketfd,ACK_ERROR_packet,clientaddr,block,ERROR,ACK_ERROR_size);
            continue;
        }
        //if checksum is ok,check opcode
        else if(ACK_ERROR_packet->mf_opcode == ACK){
            if(ACK_ERROR_packet->mf_block == 0 && ACK_ERROR_packet->mf_opcode != FRQ ){
                //this means finish the transmission
                break;
            }
            if (ACK_ERROR_packet->mf_block != block){
                //this is old packet due to time out,discard it,wait for expected packet
                continue;
            }
            else if(ACK_ERROR_packet->mf_block == block){
                //this packet is ok
                block++;
                send_packet(socketfd,ACK_ERROR_packet,clientaddr,block,ERROR,ACK_ERROR_size);
            }
        }
        else if(ACK_ERROR_packet->mf_opcode == ERROR){
            //resend previous packet again
            if(send_packet(socketfd,data_packet,clientaddr,block,DATA,data_packet_size) == -1){
                exit(1);
            }
            continue;

        }
        //ACK data is ok,send next data
    }

    /*printf("%lu bytes sent\n", index);*/
    printf("file transmission finish!!\n");
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
    else{
        struct stat buf;
        if(lstat(bootInfo.filename, &buf) < 0) 
        {
            printf("receive a request of unknow file : %s\n", bootInfo.filename);
            return FILE_NOT_EXIST;
        }
        strcpy(filename,bootInfo.filename);
        return 1;
    }

    return 0;
}
