#include"myftp.h"

unsigned short checksum(unsigned short *section, int len){
	int sum = 0;
	unsigned short *sec;
	unsigned short temp;
	unsigned short result = 0;

	sec = section;

	while(len>1){
		sum += *sec++;
		len -= 2;
	}

	if(len==1){
		*(unsigned char *)&temp = *(unsigned char *)sec;
		sum += temp;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	result = ~sum;

	return result;
}

int main(int argc,char* argv[]){

	

	if(argc!=2){
		printf("usage: ./myftpClient <port>\n");
		exit(0);
	}

	if(atoi(argv[1])<0 || atoi(argv[1])>65535){
		printf("usage: ./myftpClient <port>\n");
		exit(0);
	}

	int Server_port = atoi(argv[1]);
	int socketfd;
	if((socketfd=socket(AF_INET,SOCK_DGRAM,0))<0){
		printf("Socket error!\n");
		exit(1);
	}

	int flag=1;
	if(setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST,&flag,sizeof(int))<0){
		printf("setsockopt error!\n");
		exit(1);
	}

	struct sockaddr_in clientsock;
	bzero(&clientsock,sizeof(clientsock));
	clientsock.sin_family = AF_INET;
	clientsock.sin_port = htons(Server_port);
	clientsock.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	struct timeval timeout;
	bzero(&timeout,sizeof(struct timeval));

	timeout.tv_sec=3;
	timeout.tv_usec=0;

	int len=sizeof(struct sockaddr_in);
	struct bootServerInfo bootInfo;
	bzero(&bootInfo,sizeof(struct bootServerInfo));

	if(sendto(socketfd,&bootInfo,sizeof(struct bootServerInfo),0,(struct sockaddr *)&clientsock,len)<0){
		printf("sendto error!\n");
		exit(1);
	}

	while(1){
		fd_set settime;
		FD_ZERO(&settime);
		FD_SET(socketfd,&settime);
		int s;
		s = select(socketfd+1, &settime, NULL, NULL, &timeout);
		if(s < 0){
			perror("select error");
			break;
		}
		else if(s == 0){
			printf("no server answer!\n");
			exit(1);
		}
		else{
			printf("connect to server!\n");
			break;
		}
	}

	//receive from server
	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(struct sockaddr_in));

	int rev;
	rev = recvfrom(socketfd,&bootInfo,sizeof(bootInfo),0,(struct sockaddr*)&serveraddr,&len);
	if(rev<0){
		printf("recv package error \n");
	}
	else{
		printf("find myftpServer IP : %s\n",bootInfo.servAddr);
		printf("Myftp connent Port:%d\n",bootInfo.connectPort);
	}

	struct sockaddr_in recaddr;
	recaddr.sin_family = AF_INET;
	recaddr.sin_port = htons(bootInfo.connectPort);
	recaddr.sin_addr.s_addr = inet_addr(bootInfo.servAddr);
	int reclientsocket = socket(AF_INET,SOCK_DGRAM,0);
	if(reclientsocket == -1){
			perror("Socket Error!");
			exit(1);
	}

	int block;
	size_t totalsize;

	FILE *fin=NULL;
	char file[FNAMELEN];

	while(1){
		while(1){
			block=0;
			totalsize=0;
			printf("(1). File Get.\n");
			printf("(2). Exit.\n");
			printf("Enter a number : ");
			char command[FNAMELEN];
			fgets(command,FNAMELEN,stdin);
			if(strcmp(command,"2\n")==0){
				close(reclientsocket);
				exit(0);
			}
			else if(strcmp(command,"1\n")!=0){
				printf("Enter a number : 1 or 2 \n");
			}
			else{
				printf("File Request : ");
				fgets(command,FNAMELEN,stdin);
				command[strlen(command)-1]='\0';

				struct myFtphdr *FirstREQ;
				FirstREQ =(struct myFtphdr *)malloc(MFMAXDATA+6);
				memset(FirstREQ,0,MFMAXDATA+6);
				strcpy(FirstREQ->mf_filename,command);
				FirstREQ->mf_opcode = htons(REQ);
				FirstREQ->mf_cksum=htons(0);
				FirstREQ->mf_cksum=checksum((unsigned short *)FirstREQ,MFMAXDATA+6);
				int send = sendto(reclientsocket,FirstREQ,518,0,(struct sockaddr *)&recaddr,sizeof(struct sockaddr_in));
				if(send == -1){
					perror("Send error!");
					exit(1);
				}

				while(1){
					fd_set settime;
					FD_ZERO(&settime);
					FD_SET(reclientsocket,&settime);
					int s;
					s = select(reclientsocket+1, &settime, NULL, NULL, &timeout);
					if(s < 0){
						perror("select error");
						break;
					}
					else if(s == 0){
						printf("no server answer!\n");
						exit(1);
					}
					else{
						break;
					}
				}
			
				while(1){
					struct myFtphdr *FirstRES;
					FirstRES =(struct myFtphdr *)malloc(MFMAXDATA+6);
					rev = recvfrom(reclientsocket,FirstRES,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,&len);
					if(rev<0){
						printf("recv package error \n");
					}
					else{
						if(checksum((unsigned short *)FirstRES,MFMAXDATA+6)!=0){
							memset(FirstREQ,0,MFMAXDATA+6);
							FirstREQ->mf_opcode=htons(ERR);
							FirstREQ->mf_block=block;
							FirstREQ->mf_cksum=0;
							FirstREQ->mf_cksum=checksum((unsigned short *)FirstREQ,MFMAXDATA+6);
							if(sendto(reclientsocket,FirstREQ,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
								printf("sendto error!\n");
							}
							continue;
						}
						else if(ntohs(FirstRES->mf_opcode)==ERR){
							FirstREQ =(struct myFtphdr *)malloc(MFMAXDATA+6);
							memset(FirstREQ,0,MFMAXDATA+6);
							strcpy(FirstREQ->mf_filename,command);
							FirstREQ->mf_opcode = htons(REQ);
							FirstREQ->mf_cksum=htons(0);
							FirstREQ->mf_cksum=checksum((unsigned short *)FirstREQ,MFMAXDATA+6);
							if(sendto(reclientsocket,FirstREQ,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
								printf("sendto error!\n");
							}
							continue;
						}
						else if(ntohs(FirstRES->mf_opcode)==RES){
							if(FirstRES->mf_ans==OK){
								printf("File transmission start!!\n");
								printf("Get file : <%s> from %s\n",command,inet_ntoa(recaddr.sin_addr));
								strcpy(file,command);
								fin=fopen(strcat(file,".client"),"w+");
								totalsize += (fwrite(FirstRES->mf_data,1,strlen(FirstRES->mf_data),fin));
								block = 1;
								struct myFtphdr *ack;
								ack = (struct myFtphdr *)malloc(MFMAXDATA+6);
								ack->mf_opcode = htons(ACK);
								ack->mf_block = block;
								ack->mf_cksum=htons(0);
								ack->mf_cksum=checksum((unsigned short *)ack,MFMAXDATA+6);
								if(sendto(reclientsocket,ack,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
									printf("sendto error!\n");
								}
								while(1){
									struct myFtphdr *data;
									data = (struct myFtphdr *)malloc(MFMAXDATA+6);
									rev = recvfrom(reclientsocket,data,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,&len);
									if(rev<0){
										printf("recv package error \n");
									}
									else{
										if(checksum((unsigned short *)data,MFMAXDATA+6)!=0){
											memset(ack,0,MFMAXDATA+6);
											ack->mf_opcode=htons(ERR);
											ack->mf_block=block;
											ack->mf_cksum=0;
											ack->mf_cksum=checksum((unsigned short *)ack,MFMAXDATA+6);
											if(sendto(reclientsocket,ack,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
												printf("sendto error!\n");
											}
											continue;
										}
										else if(ntohs(data->mf_opcode)==FIN){
											printf("%d bytes received\n",totalsize);
											printf("File transmission finish!!\n\n");
											if(fin!=NULL)
												fclose(fin);
											break;
										}
										else if(data->mf_block==0){
											totalsize += (fwrite(data->mf_data,1,strlen(data->mf_data),fin));
											memset(ack,0,MFMAXDATA+6);
											ack->mf_opcode = htons(ACK);
											ack->mf_block = 0;
											ack->mf_cksum=htons(0);
											ack->mf_cksum=checksum((unsigned short *)ack,MFMAXDATA+6);
											if(sendto(reclientsocket,ack,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
												printf("sendto error!\n");
											}
											continue;
										}
										else if(ntohs(data->mf_opcode)==ERR){
											if(sendto(reclientsocket,ack,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
												printf("sendto error!\n");
											}
											continue;
										}
										else if(ntohs(data->mf_opcode)==DAT){
											totalsize += (fwrite(data->mf_data,1,strlen(data->mf_data),fin));
											block++;
											memset(ack,0,MFMAXDATA+6);
											ack->mf_opcode = htons(ACK);
											//if(data->mf_block==0)
											//	ack->mf_block = 0;
											//else
												ack->mf_block = block;
											ack->mf_cksum=htons(0);
											ack->mf_cksum=checksum((unsigned short *)ack,MFMAXDATA+6);
											if(sendto(reclientsocket,ack,MFMAXDATA+6,0,(struct sockaddr *)&recaddr,len)<0){
												printf("sendto error!\n");
											}
											continue;
										}
											
									}
								}
								break;
							}
							else{
								printf("No such file!\n\n");
								break;
							}
						}
					}
				}
			}			
		}

	}

	return 0;
}
