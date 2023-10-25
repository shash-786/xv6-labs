#include "kernel/types.h"
#include "user/user.h"

void exec_pipe(int fd) {
	int num;
	if(read(fd,&num,sizeof(int)) <= 0) {
		close(fd);
		return;
	}

	printf("prime %d\n",num);
	int temp = -1;

	int pipe2[2];
	pipe(pipe2);

	for(;;) {
		if(read(fd,&temp,sizeof(int)) <= 0) {
			break;
		}

		else if(temp % num != 0){
			write(pipe2[1],&temp,sizeof(int));
		}
	}

	if(temp == -1) {
		close(pipe2[1]);
		close(pipe2[0]);
		close(fd);
		return;
	}

	int pid = fork();

	if(pid == 0) {
		close(pipe2[1]);
		close(fd);
		exec_pipe(pipe2[0]);
		close(pipe2[0]);
	}

	else{
		close(pipe2[1]);
		close(pipe2[0]);
		close(fd);
	}
}


int main(int argc, char *argv[]) {
	if(argc > 1) {
		fprintf(2,"invalid!\n");
		exit(1);
	}

	int p[2];
	pipe(p);

	for(int i=2; i<35; i++) {
		int n = i;
		write(p[1],&n,sizeof(int));
	}

	close(p[1]);
	exec_pipe(p[0]);
	close(p[0]);
	sleep(2);
	return 0;
}
