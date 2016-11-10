#include <stdio.h>
#include <string.h>

struct tp{
	int a;
	char msg[1000];
	char *x;
};

int main(int argc,char *argv[]){
	struct tp i;
	printf("tp:%lu\n", sizeof(struct tp));
	printf("int:%lu\n", sizeof(int));
	printf("char *:%lu\n", sizeof(char *));
	printf("i:%lu\n", sizeof(i));
	char h[100] = "Hello wordl";
	char w[100] = "hungry twins";
	i.a = 5;
	// i.msg = h;
	strcpy(i.msg,h);
	i.x = &w[0];

	return 0;
}