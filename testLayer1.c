#include "globals.h"
#include "layer0.h"
#include "layer1.h"

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
#define S_IXOTH		1

int main(int argc, const char **argv){
<<<<<<< HEAD
	total_blocks = 5;
	disk = malloc(BLOCK_SIZE * 5);
	uint8_t testBuf[BLOCK_SIZE];
=======
	printf("Hello World\n");
	
	mkfs(1139);
>>>>>>> 6ed58b65183ef4a906859b338c490ee5c53ea706
	
	inode test;
	inode_free(34);
	
	
	
	//total_blocks = 5;
	//disk = malloc(BLOCK_SIZE * 5);
	//uint8_t testBuf[BLOCK_SIZE];
	
	//readBlock(1, testBuf);
	//writeBlock(0, NULL);

	return EXIT_SUCCESS;
}

int test_mkfs(){
	int error_count = 0;
	int result = mkfs(1);
	if (result != TOTALBLOCKS_INVALID){
		fprintf(stderr, "testLayer1: test_mkfs: mkfs worked wtih 1 block.\n");
		error_count++;
	}
	
	result = mkfs(2); // what to do if mkfs is called twice?
					// throw an error or just return SUCCESS?
					
	if (result != SUCCESS){
		fprintf(stderr, "testLayer1: test_mkfs: mkfs(2) didn't return SUCCESS.\n");
		error_count++;
	}
	
	if (sb->data_block_offset > 0){
		fprintf(stderr, "testLayer1: test_mkfs: mkfs(2) allocated disk blocks.\n");
		error_count++;
		
		disk = NULL;
	}
	
	result = mkfs(2147483647);
	if (result != TOTALBLOCKS_INVALID){
		fprintf(stderr, "testLayer1: test_mkfs: mkfs worked with INT_MAX blocks.\n");
		error_count++;
	}
	disk = NULL;
	fprintf(stderr, "testLayer1: test_mfks finished with %d errors.\n", error_count);
}

int test_inode_read(){
	int error_count = 0;
	
	inode* node1 = (inode *) malloc(sizeof(inode));
	
	int result = inode_read(10, node1);
	if (result != DISC_UNINITIALIZED){
		fprintf(stderr, "testLayer1: test_inode_read: inode_read doesn't return error when disk is NULL.\n");
		error_count++;
	}
	
	int mkfs_result = mkfs(2);
	result = inode_read(10, NULL);
	if (result != READNODE_NULL){
		fprintf(stderr, "testLayer1: test_inode_read: inode_read with NULL readNode doesn't break.\n");
		error_count++;
	}
	
	result = inode_read(2147483647, node1);
	if (result != INODE_NUM_OUT_OF_RANGE){
		fprintf(stderr, "testLayer1: test_inode_read: reading the INT_MAX inode doesn't break.\n");
	}
}
int test_inode_write(){
	inode* node1 = (inode*) malloc(sizeof(inode));
	node1->mode = S_IFREG & S_IRWXU; // regular file and RWX permissions
	node1->links = 0;
	node1->owner_id = 0;
	node1->size = 200;
	node1->access_time = 0;
	node1->mod_time = 0;
	int i = 0;
	for (i = 0; i < 10; i ++){
		node1->direct_blocks[i] = 0;
	}
	node1->indirect = 0;
	node1->double_indirect = 0;
	
}