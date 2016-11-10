#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT "8887"
#define MAXBUFLEN 1000
#define W 7
#define MSS 1000

struct frame{
	int seq_num;
	char msg[1000];
};

struct ack{
	int seq_num;
	// char msg[1000];
};

int RWS=W;		//receive window size
int LAF=(W-1);	//largest acceptable frame
int lafIdx = W-1;
int LFR=0;		//last frame received
int lfrIdx = 0;
int win_recv[W] = {0};
int win_seq[W] = {0};

void * get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc,char *argv[]){
	int sockfd;
	struct addrinfo hints,*servinfo,*p;
	int rv,numbytes;
	struct sockaddr_storage client_addr;
	socklen_t addr_len = sizeof(client_addr);
	char buf[MAXBUFLEN];
	char ip[INET6_ADDRSTRLEN];

	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if((rv = getaddrinfo(NULL,PORT,&hints,&servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	for(p=servinfo;p!=NULL;p=p->ai_next){
		if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
			perror("receiver: socket");
			continue;
		}

		if(bind(sockfd,p->ai_addr,p->ai_addrlen) == -1){
			close(sockfd);
			perror("receiver:bind");
			continue;
		}
		break;
	}
	if(p==NULL){
		fprintf(stderr, "receiver: failed to bind socket\n");
	}
	freeaddrinfo(servinfo);

	struct frame f;
	int diff=0;
	struct ack ackn;
	while(1){
		printf("receiver: waiting to recvfrom on port: %s\n",PORT);
		
		if((numbytes = recvfrom(sockfd,&f,sizeof(struct frame),0,(struct sockaddr *)&client_addr,&addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}


		printf("receiver: got packet from %s\n", 
			inet_ntop(client_addr.ss_family,
				get_in_addr((struct sockaddr *)& client_addr),ip,sizeof ip));
		printf("receiver: packet is %d bytes long\n",numbytes);
		buf[numbytes] = '\0';
		printf("receiver: packet contains: %d \n",f.seq_num);

		if(f.seq_num <= LAF){
			diff = (f.seq_num - win_seq[lfrIdx]);
			win_recv[(lfrIdx+diff)%W] = 1;

			int j=(lfrIdx+1)%W;
			int flag = 0;
			while(win_recv[j] == 1){
				win_recv[j]=0;
				lfrIdx = (lfrIdx+1)%W;
				LFR++;
				j = (j+1)%W;
				flag = 1;
			}
			if(flag){
				//send acknowledgement
				ackn.seq_num = LFR;
				LAF = LFR+RWS-1;
				lafIdx = (lfrIdx-1)%W;
				if((numbytes = sendto(sockfd,&ackn,sizeof(struct ack),0,(struct sockaddr *) &client_addr,addr_len) < 0)){
					perror("sendto failed");
					return -1;
				}
			}
		}
		
	}

	close(sockfd);




}