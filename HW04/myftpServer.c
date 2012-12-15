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

void listenClient(int socketfd, struct sockaddr_in *clientaddr){
	int len = sizeof(struct sockaddr_in);
	struct bootServerInfo bootInfo;

	if(recvfrom(socketfd, &bootInfo, sizeof(bootInfo), 0, (struct sockaddr *)clientaddr, &len) < 0){
		perror("recvfrom error");
		exit(1);
	}
}

void startMyFtpServer(struct sockaddr_in *clientaddr, int Port , char *file[MAXFILES]){
	int socketfd;
	int block;
	size_t totalsize;
	int finished = 0;
	int len=sizeof(struct sockaddr_in);	
		
	struct sockaddr_in bindaddr;
	bindaddr.sin_family=AF_INET;
	bindaddr.sin_port=htons(Port);
	bindaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	
	//Create socket
	if((socketfd=socket(AF_INET,SOCK_DGRAM,0))<0){
		printf("Socket error!\n");
		exit(1);
	}
	
	//bind
	if(bind(socketfd,(struct sockaddr *)&bindaddr,len)<0){
		printf("Bind error!\n");
		exit(1);
	}

	struct timeval timeout;
	bzero(&timeout,sizeof(struct timeval));

	fd_set readfd;
	FD_ZERO(&readfd);
	FD_SET(socketfd,&readfd);

	struct myFtphdr *packet;
	packet=(struct myFtphdr *)malloc(MFMAXDATA+6);
	memset(packet,0,MFMAXDATA+6);

	FILE *fin=NULL;

	while(1){
		finished = 0;
		timeout.tv_sec=30;
		timeout.tv_usec=0;
		block=0;
		totalsize=0;

		if(select(socketfd+1,&readfd,NULL,NULL,&timeout) < 0){
			perror("Select error");
			if(fin!=NULL)
				fclose(fin);
			break;
		}
		else if(select(socketfd+1,&readfd,NULL,NULL,&timeout) == 0){
			printf("Timeout.\n");		
			if(fin!=NULL)
				fclose(fin);
			return;
		}
		else{
			memset(packet, 0, MFMAXDATA+6);
			if(recvfrom(socketfd, packet, MFMAXDATA+6, 0, (struct sockaddr *)clientaddr, &len)<0){
				printf("recvfrom error!\n");
				if(fin!=NULL)
					fclose(fin);
				break;
			}
			if(checksum((unsigned short *)packet, MFMAXDATA+6)!=0){
				//bzero(mh,MFMAXDATA+6);
				memset(packet,0,MFMAXDATA+6);
				packet->mf_opcode=htons(ERR);
				packet->mf_block=0;
				packet->mf_cksum=0;
				packet->mf_cksum=checksum((unsigned short *)packet, MFMAXDATA+6);
				if((sendto(socketfd, packet, MFMAXDATA+6, 0, (struct sockaddr *)clientaddr, len))<0){
					printf("sendto error!\n");
					return;
				}
			}
			if(ntohs(packet->mf_opcode)==REQ){
				packet->mf_opcode=htons(RES);
				int i=0;
				int check=-1;
				for(i=0;i<MAXFILES;i++){
					if(strcmp(packet->mf_filename,file[i])==0){
						check = i;					
					}
				}
				if(check==-1){
					packet->mf_ans=NOK;
					finished=1;
				}
				else{
					if((fin=fopen(packet->mf_filename,"rb"))<0){
							printf("fopen error!\n");
							exit(1);
					}
					
					printf("File transmission start\n");
					printf("Send file : <%s> to %s \n",packet->mf_filename,inet_ntoa(clientaddr->sin_addr));
					++block;
					packet->mf_ans=OK;
					totalsize+=(fread(packet->mf_data, 1, MFMAXDATA, fin));
				
					while(1){
						packet->mf_cksum=0;
						packet->mf_cksum=checksum((unsigned short *)packet, MFMAXDATA+6);

						if(sendto(socketfd, packet, MFMAXDATA+6, 0, (struct sockaddr *)clientaddr,len)<0){
							printf("sendto error!\n");
							exit(1);
						}
						memset(packet,0,MFMAXDATA+6);
						if(recvfrom(socketfd,packet,MFMAXDATA+6,0,(struct sockaddr *)clientaddr,&len)<0){
							printf("recvfrom error!\n");
							if(fin!=NULL)
								fclose(fin);
							break;
						}
						if(checksum((unsigned short *)packet,MFMAXDATA+6)!=0){
							packet->mf_opcode=htons(ERR);
							packet->mf_block=htons(0);
							packet->mf_cksum=0;
							packet->mf_cksum=checksum((unsigned short *)packet,MFMAXDATA+6);
							if(sendto(socketfd,packet,MFMAXDATA+6,0,(struct sockaddr *)clientaddr,len)<0){
								printf("sendto error!\n");
								exit(1);
							}
							if(fin!=NULL)
								fclose(fin);
							return ;
						}
						if(ntohs(packet->mf_opcode)==ERR)
							continue;
						break;
					}
					while(!feof(fin)){
						packet->mf_opcode=htons(DAT);
						++block;
						packet->mf_block=htons(block);
						totalsize+=(fread(packet->mf_data,1,MFMAXDATA,fin));
						if(feof(fin))
							packet->mf_block=htons(0);
						packet->mf_cksum=0;
						packet->mf_cksum=checksum((unsigned short *)packet,MFMAXDATA+6);
					
						while(1){
							if(sendto(socketfd,packet,MFMAXDATA+6,0,(struct sockaddr *)clientaddr,len)<0){
								printf("sendto error!\n");
								if(fin!=NULL)
									fclose(fin);
								exit(1);
							}

							memset(packet,0,MFMAXDATA+6);
							if(recvfrom(socketfd,packet,MFMAXDATA+6,0,(struct sockaddr *)clientaddr,&len)<0){
								printf("recvfrom error!\n");
								if(fin!=NULL)
									fclose(fin);
								return ;
							}
							if(checksum((unsigned short *)packet,MFMAXDATA+6)!=0){
								packet->mf_opcode=htons(ERR);
								packet->mf_block=htons(block);
								packet->mf_cksum=0;
								packet->mf_cksum=checksum((unsigned short *)packet,MFMAXDATA+6);
								if(sendto(socketfd,packet,MFMAXDATA+6,0,(struct sockaddr *)clientaddr,len)<0){
									printf("sendto error!\n");
									if(fin!=NULL)
										fclose(fin);
									exit(1);
								}
								if(fin!=NULL)
									fclose(fin);
								return ;
							}
							if(ntohs(packet->mf_opcode)==ERR)
								continue;
							break;
						}
					}
					if(fin!=NULL)
						fclose(fin);
					packet->mf_opcode=htons(FIN);
					printf("%d bytes sent\n",totalsize);
					printf("File transmission finish!!\n\n");
					finished=1;
				}
				packet->mf_cksum=0;
				packet->mf_cksum=checksum((unsigned short *)packet,MFMAXDATA+6);
				if(sendto(socketfd,packet,MFMAXDATA+6,0,(struct sockaddr *)clientaddr,len)<0){
					printf("sendto error!\n");
					exit(1);
				}
				if(finished==1)
					break;
			}
			else
				break;
		}		
	}
}

int main(int argc,char* argv[]){

	if(argc!=2){
		printf("usage: ./myftpServer <port>\n");
		exit(0);
	}

	if(atoi(argv[1])<0 || atoi(argv[1])>65535){
		printf("usage: ./myftpServer <port>\n");
		exit(0);
	}

	int Server_port = atoi(argv[1]);
	int socketfd;
	if((socketfd=socket(AF_INET,SOCK_DGRAM,0))<0){
		printf("Socket error!\n");
		exit(1);
	}
	
	struct sockaddr_in serversocket;
    	bzero(&serversocket,sizeof(serversocket));
    	serversocket.sin_family=AF_INET;
    	serversocket.sin_port= htons(Server_port);
    	serversocket.sin_addr.s_addr=htonl(INADDR_ANY);
	if(setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, DEVICE, sizeof(DEVICE)) < 0){
		     close(socketfd);
		     printf("setsockopt Error\n");
		     exit(1);
	}

	if(bind(socketfd, (struct sockaddr *)&serversocket, sizeof(struct sockaddr_in)) < 0){
		printf("Bind error!\n");
		exit(1);
	}

	struct ifreq interface;
	bzero(&interface,sizeof(struct ifreq));
	strcpy(interface.ifr_name,DEVICE);
	if(ioctl(socketfd,SIOCGIFADDR,&interface)<0){
		perror("ioctl error!");
		exit(1);
	}

	struct sockaddr_in serveraddr;
	bzero(&serveraddr,sizeof(struct sockaddr_in));
	serveraddr = *((struct sockaddr_in*)&interface.ifr_addr);
	printf("Server IP : %s\n", inet_ntoa(serveraddr.sin_addr));
	printf("Myftp server start!\n");

	char *file[MAXFILES];
	file[0]=FILES1;
	file[1]=FILES2;

	struct sockaddr_in clientaddr;
	bzero(&clientaddr,sizeof(struct sockaddr_in));
	int clientaddr_len = sizeof(struct sockaddr_in);

	struct bootServerInfo bootInfo;
	bzero(&bootInfo,sizeof(struct bootServerInfo));

	while(1){
		printf("Share file : ");
		int i=0;
		for(i=0;i<MAXFILES;i++){
			printf("<%s> ",file[i]);
		}
		printf("\nWait client! ......\n\n");		

		bzero(&bootInfo,sizeof(struct bootServerInfo));

		listenClient(socketfd, &clientaddr);

		char *ClientIp = inet_ntoa(clientaddr.sin_addr);
                printf("Client from %s connect!!\n",ClientIp);
		
		int RandomPort=33000;
		srand(time(NULL));
		RandomPort = RandomPort + (rand()%1000);
		bootInfo.connectPort=RandomPort;
		strcpy(bootInfo.servAddr,inet_ntoa(serveraddr.sin_addr));
		printf("Myftp connect port : %d\n",bootInfo.connectPort);

		if((sendto(socketfd, &bootInfo, sizeof(bootInfo), 0, (struct sockaddr *)&clientaddr, sizeof(struct sockaddr_in)))<0){
			perror("sendto error");
			exit(1);
		}
		pid_t pid;
		if((pid=fork())==0){
			clientaddr.sin_port=htons(RandomPort);
			startMyFtpServer(&clientaddr,RandomPort,file);
		}
		
		else
			continue;		  
	}
	return 0;
}
