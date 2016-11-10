// sender is the client program that sends packets to the server
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define CPORT "8888"
#define MAXMSGSIZE 1000
#define W 7

struct frame{
	int seq_num;
	char msg[1000];
};

struct ack{
	int seq_num;
	// char msg[1000];
};

// char PORT[16];
int PORT;
char IP[INET6_ADDRSTRLEN];

//global vars
int SWS = W;	//send_window_size
int win_seq[W] = {0};	//what are occupied in window by what seq number
int win_recv[W] = {0};	//what are occupied in window by what seq number
int LAR = -1;	//sequence number of last acknoledgment received
int larIdx = -1;
int LFS = -1;	//seqnumber of last frame sent
int lfsIdx=-1;

int main(int argc,char *argv[]){
	//intialise
	if(argc!=3){
		printf("Usage: ./sender IP Port\n");
		return -1;
	}
	PORT = atoi(argv[2]);
	printf("Connecting to %s:%d\n",argv[1],PORT);

	//vars
	int sockfd;
	struct addrinfo hints,*clientinfo,*pc;
	struct sockaddr_in servAddr;
	char msg[MAXMSGSIZE];
	int numbytes;

	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;		//use my ip

	int rv;
	if((rv = getaddrinfo(NULL,CPORT,&hints,&clientinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	for(pc=clientinfo;pc!=NULL;pc=pc->ai_next){
		if((sockfd = socket(pc->ai_family,pc->ai_socktype,pc->ai_protocol)) == -1){
			perror("sender: socket");
			continue;
		}
		if(bind(sockfd,pc->ai_addr,pc->ai_addrlen) == -1){
			close(sockfd);
			perror("sender:bind");
			continue;
		}
		break;
	}
	if(pc==NULL){
		fprintf(stderr, "sender: failed to bind socket\n");
		return 2;
	}

    memset((char*) &servAddr,0,sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(PORT);	//htons(argv[2]);

    int i=0, seq_num = 0;
    struct frame window[W];
    int buffered = 0,pos=0;
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 500000;
    struct ack ackn;
    int byteNum = 0,diff=0;
    while(byteNum<10000){
    	while(LFS-LAR<SWS){
    		lfsIdx = (lfsIdx+1)%W;
    		LFS++;
			struct frame *f = &window[lfsIdx];
			f->seq_num = LFS;
			if((numbytes = sendto(sockfd,f,strlen(msg),0,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)){
				perror("sendto failed");
				return -1;
			}
    	}
    	
    	FD_ZERO(&readfds);
    	FD_SET(sockfd,&readfds);

    	//dont care abt write and exceptfds
    	select(sockfd+1,&readfds,NULL,NULL,&tv);
    	if(FD_ISSET(sockfd,&readfds)){
    		if((numbytes = recvfrom(sockfd,&ackn,sizeof(struct ack),0,0,0)) == -1){
				perror("recvfrom");
				exit(1);
			}
			printf("ACK %d received\n",ackn.seq_num);
		
			if(ackn.seq_num<=LFS){
				diff = ackn.seq_num - LAR;
				win_recv[(larIdx+diff)%W] = 1;
				int j = (larIdx+1)%W;
				while(win_recv[j]==1){
					win_recv[j]==0;
					larIdx = (larIdx+1)%W;
					j = (j+1)%W;
					byteNum+=1000;
				}
			}
		}
		i++;
	}

	return 0;
}