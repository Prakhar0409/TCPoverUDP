// sender is the client program that sends packets to the server
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <time.h>

#define CPORT "8888"
#define MAXSEGSIZE 1000
#define TOTMSGLENGTH 100000
#define MW 100


struct frame{
	int seq_num;			//byte number of the first byte sent
	int advt_seq_num;		//advtertised seq number - all bytes upto this are valid
	int msg_len;			//length of valid message being sent
	char msg[MAXSEGSIZE];			//message
	int ack_valid;			
};

struct data{
	int seq_num;
	int msg_len;
	time_t ts;
};

struct queue{
	struct data line[105];
	int first;
	int curr;		// block after last
	int size;
	int len;
};

int W = 5900;
int PORT;
char IP[INET6_ADDRSTRLEN];
float loss;

int main(int argc,char *argv[]){
	//intialise
	if(argc!=4){
		printf("Usage: ./sender IP Port loss\n");
		return -1;
	}
	PORT = atoi(argv[2]);
	printf("Connecting to %s:%d\n",argv[1],PORT);
	loss = atof(argv[3]);
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
    struct frame *window[MW];

    for(i=0;i<MW;i++){window[i]=NULL;}
    fd_set readfds;
    int window_size = 1;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int flag_packet_lost = 1;
    struct queue q;
    q.size = 101;
    q.first = 0;
    q.curr = 0;
    q.len = 0;
	time_t tstart,tnow; time(&tstart);
	
    while(lba < TOTMSGLENGTH-1){
    	
    	
    	time(&tnow);
    	int seconds = difftime(tnow,q.line[q.first].ts);
    	// printf("diff: %d || q_len: %d\n",seconds,q.len);
    	if(q.len>0 && seconds >= 1){
    		//timer expired - resend the packet just beyond the last ack
    		printf("REsending Packet: %d\n", q.line[q.first].seq_num);
    		struct frame *f = (struct frame *)malloc(sizeof(struct frame));
    		f->seq_num = lba+1;
    		msg_len = MAXSEGSIZE;
    		if( lba+1 + MAXSEGSIZE >= TOTMSGLENGTH){
    			msg_len = TOTMSGLENGTH-1 -lba;
    		}
    		memcpy(f->msg,&msg[lba+1],msg_len);
    		f->msg_len=msg_len;
    		f->msg[msg_len]='\0';
    		float r = (double)rand() / (double)RAND_MAX;
    		if(r>=loss){
    			printf("Really REsending packet: %d\n",f->seq_num);
	    		if((numbytes = sendto(sockfd,f,sizeof(struct frame),0,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)){
					perror("sendto failed");
					return -1;
				}
			}
			q.line[q.first].seq_num = f->seq_num;
			q.line[q.first].msg_len = f->msg_len;
			time(&(q.line[q.first].ts));
			// printf("time: %s\n",asctime(q.line[q.first].ts));
			// printf("seconds ->now: %.f\n", difftime(q.line[q.first].ts, (time_t) 0));

			//window size 1;
			int j= (lbaIdx+1)%window_size;
			for (; j != lbsIdx; j = (j+1)%window_size){
				if(window[j] != NULL){free(window[j]); window[j]=NULL;}
			}
			if(window[j] != NULL){free(window[j]); window[j]=NULL;}
			
			W = MAXSEGSIZE;
			window_size = W/MAXSEGSIZE;
			window[0] = f;
			outstanding = 1;
			lbaIdx = -1;
			lbsIdx = 0;
			// lbsIdx = lbaIdx+1
    	}
    	

    	while(outstanding < window_size && lba < TOTMSGLENGTH-1){
    		struct frame *f = (struct frame *)malloc(sizeof(struct frame));
    		f->seq_num = lbs+1;
    		msg_len = MAXSEGSIZE;
    		if( lbs+1 + MAXSEGSIZE >= TOTMSGLENGTH){
    			msg_len = TOTMSGLENGTH-1 -lbs;
    		}
    		memcpy(f->msg,&msg[lbs+1],msg_len);
    		f->msg_len=msg_len;
    		f->msg[msg_len]='\0';

    		float r = (double)rand() / (double)RAND_MAX;
    		if(r>=loss){
    			printf("Really sending packet: %d\n",f->seq_num);
	    		if((numbytes = sendto(sockfd,f,sizeof(struct frame),0,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)){
					perror("sendto failed");
					return -1;
				}
			}
			if(q.len<q.size){
				struct data d = q.line[q.curr];
				d.seq_num = f->seq_num;
				d.msg_len = f->msg_len;
				time(&d.ts);
				q.line[q.curr] = d;

				q.curr++; q.len++;
			}else{
				printf("Sender queue full\n");
			}
			// printf("Sent packet with seqNum: %d and len: %d and msg:%s\n",lbs+1,f->msg_len,f->msg);
			// printf("Sent packet with seqNum: %d and len: %d \n",lbs+1,f->msg_len);
			time(&tnow);
			seconds = difftime(tnow,tstart);
			printf("Sent: W= %d,  t_sec= %d,  seq_num= %d\n", W,seconds,f->seq_num);
			lbs += msg_len;
    		outstanding++;
			lbsIdx = (lbsIdx+1)%window_size;
			window[lbsIdx] = f;

    	}
    	
    	FD_ZERO(&readfds);
    	FD_SET(sockfd,&readfds);

    	// printf("Before timer\n");
    	select(sockfd+1,&readfds,NULL,NULL,&tv);
    	// printf("After timer\n");
    	if(FD_ISSET(sockfd,&readfds)){
    		struct frame *f = (struct frame *) malloc(sizeof(struct frame));
    		if((numbytes = recvfrom(sockfd,f,sizeof(struct frame),0,0,0)) == -1){
				perror("recvfrom");
				exit(1);
			}
			printf("Received an ack advt: %d, lba: %d, lbaIdx: %d\n",f->advt_seq_num,lba,lbaIdx);
			
			time(&tnow);
			seconds = difftime(tnow,tstart);
			
			int j= (lbaIdx+1)%window_size;
			int flag_increase_window = 0;	
			//removing from the senders window
			while(window[j]!=NULL && window[j]->seq_num < f->advt_seq_num){
				if(window[j] != NULL){
					// printf("not null\n");
					// printf("%d\n",window[j]->seq_num );
					free(window[j]);
					window[j]=NULL;
					// printf("after free\n");
				}
				lbaIdx = (lbaIdx+1)%window_size;
				j = (lbaIdx+1)%window_size;
				outstanding--;
				lba = f->advt_seq_num-1;
				flag_increase_window = 1;
			}

			if(flag_increase_window){
				int tmp = (MAXSEGSIZE*MAXSEGSIZE)/W;
				W += tmp;
				window_size = (W/MAXSEGSIZE);
				flag_increase_window = 0;
			}

			printf("ACK Received: W= %d,  t_sec= %d,  advt_seq_num= %d\n", W,seconds,f->advt_seq_num);

			free(f);
			f=NULL;
			
			//remove all timers uptill the lba
			int q_tmp = q.first;
    		while(lba >= q.line[q_tmp].seq_num && q.first != q.curr){
	    		q.len--;
	    		q.first = (q.first+1)%q.size; q_tmp = (q_tmp+1)%q.size;
	    	}
		}
		
	}

	return 0;
}