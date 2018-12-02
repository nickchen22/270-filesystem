#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define TEST_FAILED 0
#define TEST_PASSED 1

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int simple_write_read();

int main(int argc, const char **argv){
	int (*tests[])() = {simple_write_read};
	int max_test = sizeof(tests) / sizeof(tests[0]);
	int num_tests = MAX(max_test, argc - 1);
	srand(time(NULL));
	
	/* Figure out which tests to run by reading arguments */
	int i;
	int tests_to_run[num_tests];
	if (argc != 1){
		for (i = 0; i < argc - 1; i++){
			tests_to_run[i] = atoi(argv[i + 1]);
		}
		if (i < num_tests){
			tests_to_run[i] = -1;
		}
	}
	else{ /* No arguments given, so run them all */
		for (i = 0; i < num_tests; i++){
			tests_to_run[i] = i;
		}
	}
	
	/* Run the tests and print the results */
	int result, test_num;
	for (i = 0; i < num_tests; i++){
		test_num = tests_to_run[i];
		if (test_num < 0){
			break;
		}
		else if (test_num >= max_test){
			printf("TEST #% 5d: test not found\n", test_num);
			continue;
		}
		
		printf("TEST #% 5d: ", test_num);
		result = tests[test_num]();
		printf(": %s\n", result ? "PASSED" : "FAILED");
	}


	/* Any other temporary test code can go here */
	/* Any other temporary test code can go here */
	return 0;
}

int simple_write_read(){
	printf("%30s", "SIMPLE_RW");
	fflush(stdout);
	
	int fd = open("test1", O_RDWR | O_CREAT, S_IRWXU);
	const char test_buf[12] = "hello world";
	char read_buf[12];
	
	write(fd, "hello world", 12);
	lseek(fd, 0, SEEK_SET);
	read(fd, read_buf, 12);
	
	printf("file %s\n", read_buf);
	
	//unlink("test1");
	
	if (strncmp(read_buf, test_buf, 12) == 0){
		return TEST_PASSED;
	}
	
	return TEST_FAILED;
}