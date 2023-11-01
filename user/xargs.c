#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


int readlines(char* args[MAXARG], int cur) {
	char buf[1024];
	int n = 0;

	while(read(0,buf+n,1) > 0) {
		if(buf[n] == '\n') {
			break;
		}

		if(n == 1023) {
	        fprintf(2, "the argument is too long...\n");
	        exit(1);
	    }
	    n++;
	}

	buf[n] = '\0';
	if(n==0) return 0;

	int offset = 0;
	while(offset < n) {
		args[cur++] = buf + offset;
		while(buf[offset] != ' ' && offset < n) {
			offset++;
		}

		while(buf[offset] == ' ' && offset < n) {
			buf[offset++] = '\0';
		}		

	}

	return cur;
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		fprintf(2,"Usage: xargs \n");
		exit(1);
	}

	char* cmd = malloc(strlen(argv[1])+1);
	char* args[MAXARG];
	strcpy(cmd,argv[1]);


	for(int i=1; i< argc; i++) {
		args[i-1] = malloc(strlen(argv[i]+1));
		strcpy(args[i-1],argv[i]);
	}


	// CMD  = ["grep"]
	// ARGS = ["grep","-name","fork"]
	int cur = 0;

	while((cur = readlines(args,argc-1)) > 0) {
		args[cur] = '\0';

		if(fork() == 0) {
			exec(cmd,args);
			exit(0);
		}
		wait(0);
	}	


	exit(0);
}