// sender is the client program that sends packets to the server
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define CPORT "8888"
#define MAXSEGSIZE 1000
#define TOTMSGLENGTH 100000
#define W 7

struct frame{
	int seq_num;			//byte number of the first byte sent
	int advt_seq_num;		//advtertised seq number - all bytes upto this are valid
	int msg_len;			//length of valid message being sent
	char msg[MAXSEGSIZE];			//message
	int ack_valid;			
};

// char PORT[16];
int PORT;
char IP[INET6_ADDRSTRLEN];

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
	int numbytes;
	char msg[TOTMSGLENGTH];
	int i=0;
	for(i=0;i<TOTMSGLENGTH;i++){msg[i]= (i%26 + 97);}

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

    
    int lba=-1,lbaIdx=-1;		//last byte acknowledged
    int lbs=-1,lbsIdx=-1;		//last byte sent
    //last byte written is the end infinite
    int outstanding = 0,msg_len=MAXSEGSIZE;
    struct frame *window[W];
    fd_set readfds;
    int window_size = W;
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 500000;
    while(lba < TOTMSGLENGTH){
    	while(outstanding < window_size && lbs < TOTMSGLENGTH-1){
    		struct frame *f = (struct frame *)malloc(sizeof(struct frame));
    		f->seq_num = lbs+1;
    		msg_len = MAXSEGSIZE;
    		if( lbs+1 + MAXSEGSIZE >= TOTMSGLENGTH){
    			msg_len = TOTMSGLENGTH-1 -lbs;
    		}
    		memcpy(f->msg,&msg[lbs+1],msg_len);
    		f->msg_len=msg_len;
    		f->msg[msg_len]='\0';

    		if((numbytes = sendto(sockfd,f,sizeof(struct frame),0,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)){
				perror("sendto failed");
				return -1;
			}
			// printf("Sent packet with seqNum: %d and len: %d and msg:%s\n",lbs+1,f->msg_len,f->msg);
			printf("Sent packet with seqNum: %d and len: %d \n",lbs+1,f->msg_len);
			lbs += msg_len;
    		outstanding++;
			lbsIdx = (lbsIdx+1)%window_size;
			window[lbsIdx] = f;

    	}
    	
    	FD_ZERO(&readfds);
    	FD_SET(sockfd,&readfds);

    	//dont care abt write and exceptfds
    	select(sockfd+1,&readfds,NULL,NULL,&tv);
    	if(FD_ISSET(sockfd,&readfds)){
    		struct frame *f = (struct frame *) malloc(sizeof(struct frame));
    		if((numbytes = recvfrom(sockfd,f,sizeof(struct frame),0,0,0)) == -1){
				perror("recvfrom");
				exit(1);
			}
			printf("ACK received. NextSeqNoExpected: %d\n",f->advt_seq_num);
			
			int j= (lbaIdx+1)%window_size;

			while(window[j]==NULL || window[j]->seq_num < f->advt_seq_num){
				free(window[j]);

				lbaIdx = (lbaIdx+1)%window_size;
				j = (lbaIdx+1)%window_size;
				outstanding--;
				lba = f->advt_seq_num-1;
			}

		}
		
	}

	return 0;
}