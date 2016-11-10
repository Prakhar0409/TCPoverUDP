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

	while(1){
		printf("receiver: waiting to recvfrom on port: %s\n",PORT);
		
		if((numbytes = recvfrom(sockfd,buf,MAXBUFLEN-1,0,(struct sockaddr *)&client_addr,&addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}
		// void * tp = get_in_addr((struct sockaddr *)& client_addr); 
		// char *tp2 = inet_ntop(client_addr.ss_family,tp,ip,sizeof ip);
		printf("receiver: got packet from %s\n", 
			inet_ntop(client_addr.ss_family,
				get_in_addr((struct sockaddr *)& client_addr),ip,sizeof ip));
		printf("receiver: packet is %d bytes long\n",numbytes);
		buf[numbytes] = '\0';
		printf("receiver: packet contains: %s \n",buf);
	}

	close(sockfd);




}