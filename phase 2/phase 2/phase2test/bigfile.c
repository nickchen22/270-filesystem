#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define BLOCKS 500000

void w();
void r();

int main(int argc, const char **argv){
	if (argc == 1){
		w();
	}
	
	if (argc == 2){
		r();
	}
}

void w(){
	int fd = open("testdir2/testbig", O_RDWR | O_CREAT, S_IRWXU);
	
	char test_buf[10000];
	
	int i, j, oneval, twoval;
	for (i = 0; i < BLOCKS; i++){
		for (j = 0; j < 10000; j += 2){
			oneval = rand() % 10;
			twoval = 10 - oneval;
			test_buf[j] = oneval;
			test_buf[j + 1] = twoval;
		}

		write(fd, test_buf, 10000);
	}
	
	close(fd);
}

void r(){
	int fd = open("testdir2/testbig", O_RDWR | O_CREAT, S_IRWXU);
	
	char test_buf[10000];
	
	int i, j, oneval, twoval;
	for (i = 0; i < BLOCKS; i++){
		read(fd, test_buf, 10000);
		
		for (j = 0; j < 10000; j += 2){
			oneval = test_buf[j];
			twoval = test_buf[j + 1];
			
			if (oneval + twoval != 10 || oneval > 10){
				printf("error found\n");
			}
		}
	}
	
	close(fd);
	printf("reached the end");
}