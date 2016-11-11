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
	int seq_num;			//byte number of the first byte sent
	int advt_seq_num;		//advtertised seq number - all bytes upto this are valid
	int msg_len;			//length of valid message being sent
	char msg[1000];			//message
	int ack_valid;			
};


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

	int lbr=-1,lbrIdx=-1;		//last byte received;
	int nbe=0,nbeIdx=0;			//next byte expected;
	struct frame *window[W];
	//last byte read is equal to the last byte recv
	int window_size = W,diff=0;
	int i = 0;
	for(i=0;i<W;i++){
		window[i]=NULL;
	}
	struct frame *f;
	while(1){

		printf("receiver: waiting to recvfrom nbe: %d where nbeIdx: %d and lbr: %d\n",nbe,nbeIdx,lbr);
		f = (struct frame *)malloc(sizeof(struct frame));
		if((numbytes = recvfrom(sockfd,f,sizeof(struct frame),0,(struct sockaddr *)&client_addr,&addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}

		// printf("receiver: got packet from %s\n",inet_ntop(client_addr.ss_family,get_in_addr((struct sockaddr *)& client_addr),ip,sizeof ip));
		// printf("receiver: packet is %d bytes long\n",numbytes);
		printf("receiver: packet seq_num: %d and len:%d\n",f->seq_num,f->msg_len);
		// int i=0;
		// for(i=0;i<1000;i++){
		// 	print
		// }
		// printf("msg: %s\n",f->msg);

		diff = (f->seq_num - nbe)/1000;
		printf("filled: %d\n",(nbeIdx+diff)%window_size );
		window[(nbeIdx+diff)%window_size] = f;
		int j = nbeIdx;
		if(f->seq_num == nbe){
			while(window[j]!=NULL){
				printf("received j: %d\n",j);
				lbr = window[j]->seq_num + window[j]->msg_len-1;
				lbrIdx = (lbrIdx+1)%window_size;
				free(window[j]);
				window[j]=NULL;
				j = (j+1)%window_size;
			}
			nbe = lbr+1;
			nbeIdx = (lbrIdx+1)%window_size;
			f = (struct frame *) malloc(sizeof(struct frame));
			f->msg_len = 0;
			f->advt_seq_num = nbe;
			f->ack_valid = 1;
			if((numbytes = sendto(sockfd,f,sizeof(struct frame),0,(struct sockaddr *) &client_addr,addr_len) < 0)){
				perror("sendto failed");
				return -1;
			}
			free(f);
		}
	}
	close(sockfd);
}