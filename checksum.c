#include "myftp.h"
int send_packet(int socketfd,struct myFtphdr *packet,struct sockaddr_in *addr,unsigned short block,short opcode,int size){
    int len = sizeof(struct sockaddr_in);
    memset(packet,0,size);
    packet->mf_opcode=htons(opcode);
    packet->mf_cksum=0;
    packet->mf_block = block;
    packet->mf_cksum=in_cksum((unsigned short *)packet,size);
    if((sendto(socketfd, packet,size ,0, (struct sockaddr *)addr, len))<0){
        errCTL("sendto");
    }
    return 0;
}
unsigned short in_cksum(unsigned short *addr, int len)
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
