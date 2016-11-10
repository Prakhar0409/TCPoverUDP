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

	if((numbytes = sendto(sockfd,msg,strlen(msg),0,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)){
		perror("sendto failed");
		return -1;
	}

	return 0;
}