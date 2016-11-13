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
#define MW 100
#define W 5900
#define MSS 1000

//Frame Structure to be sent over the network
struct frame{
	int seq_num;			//byte number of the first byte sent
	int advt_seq_num;		//advtertised seq number - all bytes upto this are valid
	int msg_len;			//length of valid message being sent
	char msg[1000];			//message
	int ack_valid;			
};

// utility function to get address from a sockaddr structure based on sa_family
void * get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc,char *argv[]){
	//variable declarations
	int sockfd;
	struct addrinfo hints,*servinfo,*p;
	int rv,numbytes;
	struct sockaddr_storage client_addr;
	socklen_t addr_len = sizeof(client_addr);
	char ip[INET6_ADDRSTRLEN];				//utility var to print ip addr

	//hints to populate servaddr
	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;		//get self address

	//get serv addr info
	if((rv = getaddrinfo(NULL,PORT,&hints,&servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}
	//choose a addr to which we can successfully bind socket
	for(p=servinfo;p!=NULL;p=p->ai_next){
		if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
			perror("receiver: socket");
			continue;
		}
		//binding socket
		if(bind(sockfd,p->ai_addr,p->ai_addrlen) == -1){
			close(sockfd);
			perror("receiver: bind");
			continue;
		}
		break;
	}
	//exit if unable to bind to any of the returned serv addresses
	if(p==NULL){
		fprintf(stderr, "receiver: failed to bind socket\n");
		exit(1);
	}
	freeaddrinfo(servinfo);		//free the linked list structure

	int lbr=-1,lbrIdx=-1;		//last byte received;
	int nbe=0,nbeIdx=0;			//next byte expected;
	struct frame *window[MW];	//space to store the received window
	//last byte read is equal to the last byte recv 
	int window_size = MW,diff=0;		//window_size
	int i = 0;
	for(i=0;i<MW;i++){
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

		//debug print IP
		// printf("receiver: got packet from %s\n",inet_ntop(client_addr.ss_family,get_in_addr((struct sockaddr *)& client_addr),ip,sizeof ip));
		
		printf("receiver: packet seq_num: %d and len:%d\n",f->seq_num,f->msg_len);
		// printf("msg: %s\n",f->msg);

		diff = (f->seq_num - nbe)/1000;			//calculate the frame number arrived
		// printf("filled: %d\n",(nbeIdx+diff)%window_size );
		window[(nbeIdx+diff)%window_size] = f;
		int j = nbeIdx;
		if(f->seq_num == nbe){
			while(window[j]!=NULL){
				// printf("received j: %d\n",j);
				lbr = window[j]->seq_num + window[j]->msg_len-1;
				lbrIdx = (lbrIdx+1)%window_size;
				free(window[j]);
				window[j]=NULL;
				j = (j+1)%window_size;
			}
			nbe = lbr+1;
			nbeIdx = (lbrIdx+1)%window_size;
		}

		//sending acknowledgement
		struct frame *f1 = (struct frame *) malloc(sizeof(struct frame));
		f1->msg_len = 0;
		f1->advt_seq_num = nbe;
		f1->ack_valid = 1;
		if((numbytes = sendto(sockfd,f1,sizeof(struct frame),0,(struct sockaddr *) &client_addr,addr_len) < 0)){
			perror("sendto failed");
			return -1;
		}
		free(f1);
	}
	//close socket when error or done
	close(sockfd);
}