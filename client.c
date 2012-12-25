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
	struct bootServerInfo bootInfo;
    /*memcpy(bootInfo.filename,filename,strlen(filename)+1);*/
    strcpy(bootInfo.filename,filename);
    int len = sizeof(struct sockaddr_in);
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


    printf("filename = %s\n",filename);
    //receive from server
	bzero(servaddr,sizeof(struct sockaddr_in));
    int s = recvfrom(socketfd,&bootInfo,sizeof(struct bootServerInfo),MSG_WAITALL,(struct sockaddr*)servaddr,&len);
    if(s == -1){
        printf("receive server response time out!!\n");
        exit(1);
    }
    else{
        printf("find myftpServer IP : %s\n",bootInfo.servAddr);
        if(bootInfo.filename[0] != '\0'){
            printf("Myftp connent Port:%d\n",bootInfo.connectPort);
            servaddr->sin_port = htons(bootInfo.connectPort);
            /*printf("serv port = %d\n",ntohs(servaddr->sin_port));*/
        }
        else if(bootInfo.filename[0] == '\0'){
            printf("No filename \"%s\" in the myftpServer\n",filename);
            exit(1);
        }
    }

    return 0;
}


int startMyftpClient(struct sockaddr_in *servaddr, const char *filename)
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

    struct myFtphdr *packet_FRQ;
    int f_len = strlen(filename)+1;
    int FRQ_size = f_len+4;
    packet_FRQ =(struct myFtphdr *)malloc(FRQ_size);
    bzero(packet_FRQ,FRQ_size);
    packet_FRQ->mf_opcode = htons(FRQ);
    packet_FRQ->mf_cksum=htons(0);
    strcpy(packet_FRQ->mf_filename,filename);
    packet_FRQ->mf_cksum=in_cksum((unsigned short *)packet_FRQ,FRQ_size);
    if(sendto(socketfd,packet_FRQ,FRQ_size,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in)) == -1){
        exit(1);
    }

    FILE *fin;
    char recv_file[FNAMELEN];
    sprintf(recv_file,"client_%s",filename);
    fin=fopen(recv_file,"wb");
    //Function: Get file
    printf("file transmission start!!\n");
    printf("get file : <%s> from IP:%s\n",filename,inet_ntoa(servaddr->sin_addr));
    //data packet
    struct myFtphdr *data_packet;
    int data_packet_size = MFMAXDATA+6;
    data_packet = (struct myFtphdr *)malloc(data_packet_size);

    //ACK and ERROR packet
    struct myFtphdr *ACK_ERROR_packet;
    int ACK_ERROR_size = 6;
    ACK_ERROR_packet = (struct myFtphdr *)malloc(ACK_ERROR_size);

    int sockaddr_len = sizeof(struct sockaddr_in);
    int block = 0;
    int recv;
    while(1){
        //MSG_WAITALL
        bzero(data_packet,data_packet_size);
        if((recv = recvfrom(socketfd,data_packet,data_packet_size,0,(struct sockaddr*)servaddr,&sockaddr_len))<0){
            //check the FRQ is arrived or not
            if(block == 0){
                //if block  == 1,this means we have not received the data block 1
                //so,the server may not receive FRQ or the block 1 has lost
                //FRQ can be regard as request for block 1
                printf("time out!! send FRQ packet again\n");
                if(sendto(socketfd,packet_FRQ,FRQ_size,0,(struct sockaddr *)servaddr,sizeof(struct sockaddr_in)) == -1){
                    exit(1);
                }
                continue;
            }
            //client has received block 1
            printf("time out waiting data,request server to resend\n");
            send_packet(socketfd,ACK_ERROR_packet,servaddr,block,ERROR,ACK_ERROR_size);
        }
        else if(in_cksum((unsigned short *)data_packet,data_packet_size)!=0){
            //receive data has error bit
            printf("received data checksum error\n");
            send_packet(socketfd,ACK_ERROR_packet,servaddr,block,ERROR,ACK_ERROR_size);
        }
        else if(ntohs(data_packet->mf_opcode) == DATA && ntohs(data_packet->mf_block) == block+1){
            printf("receive data for block = %d\n",ntohs(data_packet->mf_block));
            
            int write_bytes = recv-6;
            fwrite(data_packet->mf_data,1,write_bytes,fin);
            /*printf("write_bytes = %d\n",write_bytes);*/
            if(write_bytes<MFMAXDATA){
                printf("file transmission finish!!\n");
                block = 0;
                send_packet(socketfd,ACK_ERROR_packet,servaddr,block,ACK,ACK_ERROR_size);
                fclose(fin);
                break;
            }
            else{
            //send ACK
            block = ntohs(data_packet->mf_block);
            send_packet(socketfd,ACK_ERROR_packet,servaddr,block,ACK,ACK_ERROR_size);
            printf("send ack = %d\n",block);
            if(block == 65534){
                block = 1;
            }
            }
        }else if(ntohs(data_packet->mf_opcode) == DATA && ntohs(data_packet->mf_block) != block+1){
            //server dose not receive previous ack packet,send again
            int previous = ntohs(data_packet->mf_block);
            send_packet(socketfd,ACK_ERROR_packet,servaddr,previous,ACK,ACK_ERROR_size);
            
        }
    }

        

    return 0;
}
