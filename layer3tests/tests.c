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

#define BLOCK_SIZE 4096

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int simple_write_read();
int simple_create();
int write_read_file();

int main(int argc, const char **argv){
	int (*tests[])() = {simple_write_read, simple_create, write_read_file};
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
	
	int fd = open("testdir/test1", O_RDWR | O_CREAT, S_IRWXU);
	const char test_buf[12] = "hello world";
	char read_buf[12];
	
	write(fd, "hello world", 12);
	lseek(fd, 0, SEEK_SET);
	read(fd, read_buf, 12);
	
	unlink("testdir/test1");
	
	if (strncmp(read_buf, test_buf, 12) == 0){
		return TEST_PASSED;
	}
	
	return TEST_FAILED;
}

int simple_create(){
	printf("%30s", "SIMPLE_CREATE");
	fflush(stdout);
	
	int fd = open("testdir/test2", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	
	if (open("testdir/test2", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO) >= 0){
		unlink("testdir/test2");
		return TEST_PASSED;
	}
	
	unlink("testdir/test2");
	
	return TEST_FAILED;
}

int write_read_file(){
	printf("%30s", "WRITE_READ_FILE_RAND");
	fflush(stdout);
	
	int size = BLOCK_SIZE * (rand() % 1500);
	size += rand() % BLOCK_SIZE;
	
	char expected_result[size];
	char actual_result[size];
	
	int i;
	for (i = 0; i < size; i++){
		expected_result[i] = rand() % 256;
	}
	
	int fd = open("testdir/test3", O_RDWR | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
	//int fd = open("testdir/test3", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	write(fd, expected_result, size);
	int fd2 = open("testdir/test3", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	read(fd2, actual_result, size);
	unlink("testdir/test3");
	close(fd);
	close(fd2);
	
	
	if (memcmp(expected_result, actual_result, size) == 0){
		return TEST_PASSED;
	}

	return TEST_FAILED;
}