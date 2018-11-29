#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"
#include <limits.h>

/*
#define S_IFMT 		1507328
#define S_IFSOCK 	1310720
#define S_IFLNK 	1179648
#define S_IFREG		1048576
#define S_IFBLK		393216
#define S_IFDIR		262144
#define S_IFCHR		131072
#define S_IFIFO		65536

#define S_IRWXU		1792
#define S_IRUSR		1024
#define S_IWUSR		512
#define S_IXUSR		256

#define S_IRWXG		112
#define S_IRGRP		64
#define S_IWGRP		32
#define S_IXGRP		16

#define S_IRWXO		7
#define S_IROTH		4
#define S_IWOTH		2
#define S_IXOTH		1 */


/* 
 *	Writes the supplied inodes into the FS.
 *	Writes the inodes starting at index 1, after the root inode, to the 
 *	inode indexed at the length of the test suite minus one.
 */
void test_inode_write(inode* inodes, int numInodes) {
	int length = numInodes;
	int i;
	for (i = 0; i < length; i++) {
		int result = inode_write(i + 1, &inodes[i]);
		if (result != SUCCESS) {
			printf("^^^^^ ERROR in testing inode write with inode: %d \n", i);
		}
	}
}

/*
 *	Reads the supplied inodes from the test suite from the FS.
 * 	Uses the same indexing system implemented in the testing
 *	inode write.
*/
void test_inode_read(inode* inodes, int numInodes){
	int length = numInodes;
	inode* read_inodes = malloc(sizeof(inode) * length);
	int i;
	for (i = 0; i < length; i++) {
		int result = inode_read(i + 1, &read_inodes[i]);
		if (result != SUCCESS) {
			printf("^^^^^ ERROR in testing inode read with inode: %d \n", i);
		}

		/* The following lines will print the read inodes for human readable purposes */
		// printf("     printing inode: %d \n", i);
		// print_inode(&read_inodes[i]);
	}
}

/* 	
 * 	Declares and initializes a test suite of inodes to be run on inode write/read.
 * 	Add any additional inode tests to this test suite and additionally, update
 *	the value numInodes in tests().
 */
inode* createTestInodes(int numInodes) {
	inode* nodes = malloc(sizeof(inode) * numInodes);
	// inode 0
	nodes[0].mode = S_IFREG & S_IRWXU; // regular file and RWX permissions
	nodes[0].links = 1;
	nodes[0].owner_id = 0; // owner id corresponds to the inode in the test set for identificatin purposes
	nodes[0].size = 200;  // size of the file this inode belongs to is less than one block size
	nodes[0].access_time = 0;
	nodes[0].mod_time = 0;
	int i;
	nodes[0].direct_blocks[0] = 1;
	for (i = 1; i < 10; i ++){
		nodes[0].direct_blocks[i] = 0; // assumes addresses with 0 are unused
	}
	nodes[0].indirect = 0;
	nodes[0].double_indirect = 0;

	// inode 1
	nodes[1].mode = S_IFREG & S_IRWXU;
	nodes[1].links = 1;
	nodes[1].owner_id = 1;
	nodes[1].size = 4096; // size of the file this inode belongs to is exactly one block size
	nodes[1].access_time = 0;
	nodes[1].mod_time = 0;
	nodes[1].direct_blocks[0] = 2;
	for (i = 1; i < 10; i ++){
		nodes[1].direct_blocks[i] = 0;
	}
	nodes[1].indirect = 0;
	nodes[1].double_indirect = 0;

	// inode 2
	nodes[2].mode = S_IFREG & S_IRWXU;
	nodes[2].links = 1;
	nodes[2].owner_id = 2;
	nodes[2].size = 5000; // size of the file this inode belongs to spans two blocks
	nodes[2].access_time = 0;
	nodes[2].mod_time = 0;
	nodes[2].direct_blocks[0] = 4; // location of file are 2 blocks next to each other
	nodes[2].direct_blocks[1] = 5;
	for (i = 2; i < 10; i ++){
		nodes[2].direct_blocks[i] = 0;
	}
	nodes[2].indirect = 0;
	nodes[2].double_indirect = 0;

	//inode 3
	nodes[3].mode = S_IFREG & S_IRWXU; // regular file and RWX permissions
	nodes[3].links = 1;
	nodes[3].owner_id = 3;
	nodes[3].size = BLOCK_SIZE * 10;  // size of the file is 10 blocks
	nodes[3].access_time = 0;
	nodes[3].mod_time = 0;
	for (i = 0; i < 10; i ++){
		nodes[3].direct_blocks[i] = i + 10;
	}
	nodes[3].indirect = 0;
	nodes[3].double_indirect = 0;

	return nodes;
}

/*
 * 	Entrypoint of tests for layer 1. Tests mkfs for multiple values with emphasis on edge cases.
 * 	If created successfully, inode write tests are tested for that filesystem.
 * 	These inodes that are written, are then tested with different read calls.
 * 	Application of these inode interactions will not be meaningful until implementation
 *	of layer 2.
 */
void tests(int filesystems[]) {
	int numInodes = 4; 	// must match the number of inodes in the test suite
	inode* inodes = createTestInodes(numInodes);
	// for (int i = 0; i < numInodes; i++) {
	// 	print_inode(&inodes[i]);
	// }

	int i;
	for (i = 0; i < sizeof(filesystems); i++) {
		printf("\nBuilding filesystem of %d blocks\n", filesystems[i]);
		int result = mkfs(filesystems[i]);
		if (result != SUCCESS) {
			printf("^^^^^^ ERRORS WERE FOUND IN mkfs(%d)\n", filesystems[i]);
		}
		else {
			printf("Running inode write tests for fs of size (%d)...\n", filesystems[i]);
			test_inode_write(inodes, numInodes);
			printf("WRITE TESTS COMPLETE!\n");
			printf("Running inode read tests for fs of size (%d)...\n", filesystems[i]);
			test_inode_read(inodes, numInodes);
			printf("READ TESTS COMPLETE!\n");
		}
		free(disk); // frees the filesystem in order to prevent memory leak during testing
	}
}

int main(int argc, const char **argv){

	int fs_sizes[8]; //filesystem sizes to test in number of blocks
	fs_sizes[0] = 0;
	fs_sizes[1] = 1;
	fs_sizes[2] = 2;
	fs_sizes[3] = 3;
	fs_sizes[4] = 4;
	fs_sizes[5] = 1024;
	fs_sizes[6] = 2048;
	fs_sizes[7] = INT_MAX;

	printf("Beginning tests...\n");
	tests(fs_sizes);
	printf("Tests complete!\n");
	
	
	mkfs(400);
	
	int inode_num = 0;
	inode dummy_inode;
	inode_create(&dummy_inode, &inode_num);
	printf("%d\n", inode_num);
	inode_create(&dummy_inode, &inode_num);
	printf("%d\n", inode_num);
	inode_create(&dummy_inode, &inode_num);
	printf("%d\n", inode_num);
	inode_create(&dummy_inode, &inode_num);
	printf("%d\n", inode_num);
	
	inode_free(inode_num);
	
	inode_create(&dummy_inode, &inode_num);
	printf("%d\n", inode_num);
	
	printf("Eaccess: %d\n", EACCES);
	dirblock a;
	printf("size of dirblock: %d\n", sizeof(dirblock));

	return 0;
}