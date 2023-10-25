#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
 {
 	int p[2];
 	pipe(p);
 	char buff[10];

 	int pid = fork();

 	if(pid == 0) {
 		close(p[0]);
 		write(p[1],"ping",4);
 		printf("%d: received ping\n",getpid());
 		close(p[1]);
 	}

 	else{
 		wait(0);
 		close(p[1]);
 		read(p[0],buff,4);
 		printf("%d: received pong\n",getpid());
 		close(p[0]);
 	}

 	return 0;
 } 


 