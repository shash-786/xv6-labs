#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char* path,char* target_file) {
	
	int fd;
	struct stat st;
	struct dirent de;
	char buf[512], *p;
	
	if((fd = open(path,0)) < 0) {
		fprintf(2, "ls: cannot open %s\n", path);
    	exit(1);
	}

	if(fstat(fd,&st) < 0) {
		fprintf(2,"stat: cannot stat %s\n",fd);
		close(fd);
		exit(1);
	}

	if(st.type != T_DIR) {
		fprintf(2,"Path not a DIR!\n");
		close(fd);
		exit(1);
	}

	if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
	    fprintf(2, "find: path too long\n");
	    close(fd);
	    exit(1);
  	}	

	strcpy(buf,path);
	p = buf + strlen(buf);
	*p++ = '/';

	while(read(fd,&de,sizeof(de)) == sizeof(de)) {
		if (de.inum == 0 || !strcmp(de.name,".") || !strcmp(de.name,"..")) {
			continue;
		}

		memmove(p,de.name,DIRSIZ);
		p[DIRSIZ] = 0;
		
		if(stat(buf, &st) < 0) {
     	 	fprintf(1, "find: cannot stat %s\n", buf);
      		continue;
    	}

    	if(st.type == T_FILE && !strcmp(de.name,target_file)) {
    		printf("%s\n",buf);
    	}

    	else if (st.type == T_DIR) {
      		find(buf, target_file);
		}

	}
		close(fd);
}


int main(int argc, char* argv[]) {
	if(argc < 2 || argc > 3) {
		fprintf(2,"Usage: find incorrect arguments\n");
		exit(1);
	}

	else if(argc == 2){
		find(".",argv[1]);
	}

	else{
		find(argv[1],argv[2]);
	}	

	exit(0);
}