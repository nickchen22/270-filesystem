#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"
#include <limits.h>
#include <time.h>

#define TEST_FAILED 0
#define TEST_PASSED 1

int mkfs_size();
int lots_inodes();
int lots_inodes_free_some();
int lots_inodes_free_all();
int free_nonexistent_inode();
int get_nth_1();
int get_nth_2();
int write_read_file();
int write_read_file_offset();
int write_read_file_offset_2();
int overwrite_with_zeros();
int write_on_small_fs_1();
void print_inode(inode* inode);

int main(int argc, const char **argv){
	int (*tests[])() = {mkfs_size, lots_inodes, lots_inodes_free_some, free_nonexistent_inode, get_nth_1, get_nth_2, write_read_file, write_read_file_offset, write_read_file_offset_2, overwrite_with_zeros, write_on_small_fs_1};
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
	mkfs(400, 0, 0);
	
	dir_ent array[100];
	dir_ent d;
	strcpy(d.name, "hello world");
	
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(dummy_inode));

	struct stat s = get_stat(1);
	printf("size = %d\n", s.st_size);
	
	for (i = 34; i < 50; i++){
		d.inode_num = i;
		add_dirent(1, &d);
	}
	
	for (i = 0; i < 53; i++){
		remove_dirent(1, 400);
	}

	int entries = 0, last = 23;
	
	printf("result = %d\n", mkdir_fs("/dir", S_IRWXU | S_IRWXG | S_IRWXO, 0, 0));
	printf("result = %d\n", mknod_fs("/file", S_IRWXU | S_IRWXG | S_IRWXO, 0, 0));
	printf("result = %d\n", mkdir_fs("/dir/sdf", S_IRWXU | S_IRWXG | S_IRWXO, 0, 0));
	printf("result = %d\n", mkdir_fs("/asdf/sdf", S_IRWXU | S_IRWXG | S_IRWXO, 0, 0));
	
	oft_add(4, 6);
	oft_attempt_delete(4);
	oft_remove(10);
	
	read_dir_page(1, (dirblock*)array, 1, &entries, &last);
	
	printf("entries: %d, last: %d\n", entries, last);
	
	for (i = 0; i < entries; i++){
		printf("%s: %d\n", array[i].name, array[i].inode_num);
	}
	
	int target;
	int parent, ret, index;
	if ((ret = namei("/dir", 8, 8, &parent, &target, &index)) == SUCCESS){
		printf("namei success %d %d\n", target, index);
	}
	else{
		printf("namei failed %d\n", ret);
	}
	
	printf("oft_inodes_size = %d\n", oft_inodes_size);
	for (i = 0; i < oft_inodes_size; i++){
		printf("  inum             %d\n", oft_inodes[i].inum);
		printf("  ref              %d\n", oft_inodes[i].ref);
		printf("  pending_deletion %d\n", oft_inodes[i].pending_deletion);
	}
	
	printf("ofd_fds_size = %d\n", oft_fds_size);
	for (i = 0; i < oft_fds_size; i++){
		printf("  fd             %d\n", oft_fds[i].fd);
		printf("  inum           %d\n", oft_fds[i].inum);
		printf("  flags          %d\n", oft_fds[i].flags);
	}
	
	s = get_stat(1);
	printf("size = %d\n", s.st_size);
	printf("%x", O_RDONLY);
	
	/* Any other temporary test code can go here */
	return 0;
}

/* Prints an inode */
void print_inode(inode* inode){
	printf("inode->mode:           %d\n", inode->mode);
	printf("inode->links:          %d\n", inode->links);
	printf("inode->uid:            %d\n", inode->uid);
	printf("inode->gid:            %d\n", inode->gid);
	printf("inode->size:           %d\n", inode->size);
	printf("inode->access_time:    %d\n", inode->access_time);
	printf("inode->mod_time:       %d\n", inode->mod_time);
	printf("inode->direct_blocks:  %d",   inode->direct_blocks[0]);
	int i;
	for (i = 1; i < NUM_DIRECT; i++){
		printf(", %d", inode->direct_blocks[i]);
	}
	printf(": \n");
	printf("inode->indirect:        %d\n", inode->indirect);
	printf("inode->double_indirect: %d\n", inode->double_indirect);
}

 /********************************************************************************************************************/
 /********************************************************************************************************************/
 /*************************************************** WHAT TO TEST ***************************************************/
 /********************************************************************************************************************/
 /********************************************************************************************************************/

/* Layer 0: */

// Write data to some blocks, check to make sure we get that data back when we read them

/* Layer 1: */

// Superblock takes on valid values after a mkfs call
// Freelist bitmap is initialized to a single 1 (for root inode) followed by all 0s after mkfs
// Root directory is created and is a valid directory object with entries . and .. that point to itself
// Check limit conditions that may come up during testing -- EX don't test buffer null stuff, do test FS full stuff
// Should be able to confidently assert that all layer 1 functions work exactly as defined

// Create, delete and use inodes in a way that makes sure everything about them works
//   - make sure the values stored in inodes are persistant and aren't accidently overwritten by other calls
//   - Should maybe have a randomized test or two
//   - Load the ilistbitmap and make sure it matches which inodes are allocated/not allocated

// Allocate, free, and modify data blocks to be sure everything works
//   - Very similar to the inode section except instead of loading ilistbitmap you can walk the freelist

/* Layer 2: */

// To be determined

 /*******************************************************************************************************************/
 /*******************************************************************************************************************/
 /****************************************************** TESTS ******************************************************/
 /*******************************************************************************************************************/
 /*******************************************************************************************************************/
 
/* PURPOSE:
 *   - Confirm that mkfs properly fails for invalid block numbers
 *   - Confirm that mkfs returns SUCCESS for valid block numbers
 * METHODOLOGY:
 *   - Run mkfs with different block numbers and examine the results
 * EXPECTED RESULTS:
 *   - mkfs returns TOTALBLOCKS_INVALID for negatives, FS_TOO_SMALL when smaller than MIN_BLOCKS, SUCCESS otherwise
 */
int mkfs_size(){
	printf("%30s", "MKFS_SIZE");
	fflush(stdout);
	
	int TESTS[] = {-3, -1, 0, 1, 2, 3, 4, 5, 120, 300, 700, 1204, 2048, INT_MAX};

	int i, test_value, expected_result, actual_result;
	for (i = 0; i < sizeof(TESTS) / sizeof(int); i++){
		test_value = TESTS[i];

		if (test_value < 0){
			expected_result = TOTALBLOCKS_INVALID;
		} else if (test_value < MIN_BLOCKS){
			expected_result = FS_TOO_SMALL;
		} else {
			expected_result = SUCCESS;
		}
		
		actual_result = mkfs(test_value, 0, 0);
		if (disk != NULL){
			free(disk);
		}
		
		if (expected_result != actual_result){
			return TEST_FAILED;
		}
	}
	
	return TEST_PASSED;
}

/* PURPOSE:
 *   - Confirm that many inodes can be created
 *   - Confirm that the lowest available inode is allocated first & same inode isn't allocated multiple times if not freed
 *   - Confirm that inodes cannot be created once FS is full
 *   - Confirm that we can fit the expected number of inodes in the FS
 * METHODOLOGY:
 *   - Create inodes until the filesystem runs out of space for them
 * EXPECTED RESULTS:
 *   - inode creation works and returns SUCCESS, until the FS runs out of room and they begin returning ILIST_FULL
 *   - exactly sb.num_inodes - 1 inodes should be created, since the root takes up a spot
 */
int lots_inodes(){
	printf("%30s", "MK_INODES_UNTIL_FAILURE");
	fflush(stdout);
	
	int BLOCKS = 400;

	mkfs(BLOCKS, 0, 0);
	superblock sb;
	read_superblock(&sb);
	
	int expected_inodes = sb.num_inodes - 1;

	int testing = 1, number = 0, previous_number = 0, result = 0, inodes_created = 0;
	inode dummy_inode;
	
	while (testing){
		result = inode_create(&dummy_inode, &number);
		
		/* Check return values */
		if (result == SUCCESS){
			inodes_created++;
		}
		else if (result == ILIST_FULL){
			break;
		}
		else{
			free(disk);
			printf("%d\n", result);
			return TEST_FAILED;
		}
		
		/* Check that inode numbers are increasing */
		if (number <= previous_number){
			free(disk);
			return TEST_FAILED;
		}
		previous_number = number;
	}
	
	free(disk);
	if (inodes_created != expected_inodes){
		//printf("%d != %d\n", inodes_created, expected_inodes);
		return TEST_FAILED;
	}

	return TEST_PASSED;
}

/* PURPOSE:
 *   - Confirm that many inodes can be created
 *   - Confirm that the lowest available inode is allocated first & same inode isn't allocated multiple times if not freed
 *   - Confirm that inodes cannot be created once FS is full
 *   - Confirm that we can fit the expected number of inodes in the FS
 *	 - Confirm that inodes can be freed expectedly
 * METHODOLOGY:
 *   - Create inodes until the filesystem runs out of space for them and then free an arbitrary number of inodes
 * 	 - Frees inodes starting from index 2, after the root inode
 * EXPECTED RESULTS:
 *   - inode creation works and returns SUCCESS, until the FS runs out of room and they begin returning ILIST_FULL
 *   - exactly sb.num_inodes - 1 inodes should be created at this point
 * 	 - an arbitrary number of inodes are then freed, if result is SUCCESS, the number of inodes created is decremented
 *	 - exactly sb.num_inodes - 1 - (NUM_FREED) should exist
 */
int lots_inodes_free_some() {
	printf("%30s", "MK_INODES_AND_FREE_SOME");
	fflush(stdout);
	
	int BLOCKS = 400;
	int NUM_FREED = 10;

	mkfs(BLOCKS, 0, 0);
	superblock sb;
	read_superblock(&sb);
	
	int expected_inodes = sb.num_inodes - 1 - NUM_FREED;

	int testing = 1, number = 0, previous_number = 0, result = 0, inodes_created = 0;
	inode dummy_inode;
	
	while (testing){
		result = inode_create(&dummy_inode, &number);
		
		/* Check return values */
		if (result == SUCCESS){
			inodes_created++;
		}
		else if (result == ILIST_FULL){
			break;
		}
		else{
			free(disk);
			printf("%d\n", result);
			return TEST_FAILED;
		}
		
		/* Check that inode numbers are increasing */
		if (number <= previous_number){
			free(disk);
			return TEST_FAILED;
		}
		previous_number = number;
	}
	
	int i;
	for (int i; i < NUM_FREED; i++) {

		/* Starts free at inode after root */
		result = inode_free(i + 2);

		if (result == SUCCESS) {
			inodes_created--;
		}
		else if (result == BAD_INODE) {
			free(disk);
			printf("inode free out of range at inode num: %d\n", i);
			return TEST_FAILED;
		}
		else {
			free(disk);
			printf("%d\n", result);
			return TEST_FAILED;
		}
	}

	free(disk);
	if (inodes_created != expected_inodes){
		return TEST_FAILED;
	}

	return TEST_PASSED;
}

int lots_inodes_free_all() {
	printf("%30s", "MK_INODES_THEN_FREE");
	fflush(stdout);
	
	int BLOCKS = 400;

	mkfs(BLOCKS, 0, 0);
	superblock sb;
	read_superblock(&sb);
	
	int expected_inodes = sb.num_inodes - 1;

	int testing = 1, number = 0, previous_number = 0, result = 0, inodes_created = 0;
	inode dummy_inode;
	
	while (testing){
		result = inode_create(&dummy_inode, &number);
		
		/* Check return values */
		if (result == SUCCESS){
			inodes_created++;
		}
		else if (result == ILIST_FULL){
			break;
		}
		else{
			free(disk);
			printf("%d\n", result);
			return TEST_FAILED;
		}
		
		if (number <= previous_number){
			free(disk);
			return TEST_FAILED;
		}
		previous_number = number;
	}

	if (inodes_created != expected_inodes){
		return TEST_FAILED;
	}

	int i;
	for (i = 0; i < expected_inodes; i++) {
		int free_result = inode_free(i + 2);

		if (free_result == SUCCESS) {
			inodes_created--;
		}
		else if (result == BAD_INODE) {
			free(disk);
			printf("inode out of range: %d\n,", free_result);
			return TEST_FAILED;
		}
		else {
			free(disk);
			printf("%d\n", free_result);
			return TEST_FAILED;
		}
	}

	free(disk);
	if (inodes_created != 0)
		return TEST_FAILED;

	return TEST_PASSED;
}

/*
 * PUROPOSE:
 *		- Ensure there are no errors should an inode be freed that has not been allocated
 * METHODOLOGY:
 *		- Create a fs that will, initially, have no inodes
 *		- Free the first inode after the root inode
 *		- Create an inode which should have the first available inode num, the inode just freed
 * EXPECTED RESULTS:
 * 		- the inode should be freed correctly with no erros and report SUCCESS
 *		- an inode is created, should that success, the inode number it is assigned should be the same
 *		 	index that was just freed and return TEST_PASSED
 *		- any unexpected errors during this are printed and return TEST_FAILED
 */
int free_nonexistent_inode() {
	printf("%30s", "FREE_NONEXISTENT_INODE");
	fflush(stdout);
	
	int BLOCKS = 400;

	mkfs(BLOCKS, 0, 0);
	superblock sb;
	read_superblock(&sb);

	/* Frees the inode directly after the root that has not been allocated */
	int result = inode_free(2);

	if (result == SUCCESS) {
		int dummy_inode_num;
		inode dummy_inode;
		int create_result = inode_create(&dummy_inode, &dummy_inode_num);
		if (create_result == SUCCESS) {
			if (dummy_inode_num == 2) {
				return TEST_PASSED;
			}
			else {
				printf("Assigned incorrect inode num, index considered taken.");
				result = create_result;
			}
		}
		else {
			result = create_result;
		}
	}
	free(disk);
	printf("%d\n", result);
	return TEST_FAILED;
}

/*
 * PURPOSE:
 *   - Confirm that get_nth_datablock works as intended for reading and writing for direct and single indirect blocks
 * METHODOLOGY:
 *   - Call get_nth on a datablock with create true, then call it with create false
 * EXPECTED RESULTS:
 *   - First call to get_nth creates the data block with created set
 *   - Second call returns the same data block with created not set
 */
int get_nth_1(){
	printf("%30s", "MK_READ_DATA_BLOCKS_1");
	fflush(stdout);
	
	int test_blocks = (NUM_DIRECT + ADDRESSES_PER_BLOCK) * 5;
	int fs_blocks = test_blocks * 2;

	int number, expected, actual, created;
	mkfs(fs_blocks, 0, 0);
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));

	inode_create(&dummy_inode, &number);
	
	int i;
	for (i = 0; i < test_blocks; i++){
		created = FALSE;
		
		/* Call it twice on the same index */
		actual = get_nth_datablock(&dummy_inode, i, TRUE, &created);
		expected = get_nth_datablock(&dummy_inode, i, FALSE, &created);
		
		/* Compare the results */
		if (created == FALSE || actual != expected){
			free(disk);
			return TEST_FAILED;
		}
	}
	
	free(disk);
	return TEST_PASSED;
}

/* PURPOSE:
 *   - Confirm that get_nth_datablock works as intended for reading and writing for random blocks (mostly triple indirect)
 * METHODOLOGY:
 *   - Call get_nth on a datablock with create true, then call it with create false
 * EXPECTED RESULTS:
 *   - First call to get_nth creates the data block
 *   - Second call returns the same data block with created not set
 */
int get_nth_2(){
	printf("%30s", "MK_READ_DATA_BLOCKS_2");
	fflush(stdout);
	
	int max_n = 50000000;
	int test_blocks = 10000;
	int fs_blocks = test_blocks * 5;
	
	int n_to_use;
	int ns[test_blocks];
	int addrs[test_blocks];

	int number, created;
	mkfs(fs_blocks, 0, 0);

	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));
	inode_create(&dummy_inode, &number);
	
	/* Call it many times with random values */
	int i;
	for (i = 0; i < test_blocks; i++){
		n_to_use = rand() % max_n;
		
		ns[i] = n_to_use;
		addrs[i] = get_nth_datablock(&dummy_inode, n_to_use, TRUE, &created);
	}

	/* Re-call it with the same n values, expecting the same addr values and created = FALSE */
	created = FALSE;
	for (i = 0; i < test_blocks; i++){
		n_to_use = ns[i];

		if (get_nth_datablock(&dummy_inode, n_to_use, FALSE, &created) != addrs[i]){
			free(disk);
			return TEST_FAILED;
		}
	}
	
	if (created != FALSE){
		free(disk);
		return TEST_FAILED;
	}
	
	free(disk);
	return TEST_PASSED;
}

/* PURPOSE:
 *   - Confirm that writing a file and then immediately reading it works
 * METHODOLOGY:
 *   - Create a file with random digits, write it, and read it back
 * EXPECTED RESULTS:
 *   - The same data written will be read back
 */
int write_read_file(){
	printf("%30s", "WRITE_READ_FILE_RAND");
	fflush(stdout);
	
	mkfs(20000, 0, 0);
	
	int size = BLOCK_SIZE * (rand() % 1000);
	size += rand() % BLOCK_SIZE;
	
	uint8_t expected_result[size];
	uint8_t actual_result[size];
	
	int i;
	for (i = 0; i < size; i++){
		expected_result[i] = rand() % 256;
	}
	
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));
	
	int number;
	inode_create(&dummy_inode, &number);
	
	write_i(number, expected_result, 0, size);
	read_i(number, actual_result, 0, size);
	
	if (memcmp(expected_result, actual_result, size) == 0){
		free(disk);
		return TEST_PASSED;
	}

	free(disk);
	return TEST_FAILED;
}

/* PURPOSE:
 *   - Confirm that writing a file and then immediately reading it works when used with an offset
 * METHODOLOGY:
 *   - Create a file with random digits, write it, and read it back
 * EXPECTED RESULTS:
 *   - The same data written will be read back
 */
int write_read_file_offset(){
	printf("%30s", "WRITE_READ_FILE_RAND_OFFSET");
	fflush(stdout);
	
	mkfs(20000, 0, 0);
	
	int size = BLOCK_SIZE * (rand() % 1000);
	size += rand() % BLOCK_SIZE;
	
	int offset = rand() % size;
	
	uint8_t expected_result[size];
	uint8_t actual_result[size];
	
	int i;
	for (i = 0; i < size; i++){
		expected_result[i] = rand() % 256;
	}
	
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));
	
	int number;
	inode_create(&dummy_inode, &number);
	
	write_i(number, expected_result, 0, size);
	read_i(number, actual_result, offset, size - offset);
	
	if (memcmp((void*)((uintptr_t)expected_result + (uintptr_t)offset), actual_result, size - offset) == 0){
		free(disk);
		return TEST_PASSED;
	}
	
	free(disk);
	return TEST_FAILED;
}

int write_read_file_offset_2(){
	printf("%30s", "WRITE_READ_FILE_RAND_OFFSET_2");
	fflush(stdout);
	
	mkfs(20000, 0, 0);
	
	int size = BLOCK_SIZE * (rand() % 1000);
	size += rand() % BLOCK_SIZE;
	
	int offset = rand() % size;
	
	uint8_t expected_result[size];
	uint8_t actual_result[size];
	
	int i;
	for (i = 0; i < size; i++){
		expected_result[i] = rand() % 256;
	}
	
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));
	
	int number;
	inode_create(&dummy_inode, &number);
	
	write_i(number, expected_result, offset, size - offset);
	read_i(number, actual_result, 0, size);
	
	if (memcmp(expected_result, (void*)((uintptr_t)actual_result + (uintptr_t)offset), size - offset) == 0){
		free(disk);
		return TEST_PASSED;
	}
	
	free(disk);
	return TEST_FAILED;
}

/* PURPOSE:
 *   - Confirm that writing 0s to a file deletes it
 * METHODOLOGY:
 *   - Create a file with random digits, write to it, write 0s over previous write, check inode fields
 * EXPECTED RESULTS:
 *   - The inode will be empty
 */
int overwrite_with_zeros(){
	printf("%30s", "OVERWRITE_WITH_ZERO");
	fflush(stdout);
	
	mkfs(20000, 0, 0);
	
	int size = BLOCK_SIZE * (rand() % 30);
	size += rand() % BLOCK_SIZE;
	
	uint8_t zero_buf[size];
	memset(zero_buf, 0, size);
	
	uint8_t rand_buf[size];
	uint8_t actual_result[size];
	
	int i;
	for (i = 0; i < size; i++){
		rand_buf[i] = rand() % 256;
	}
	
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));
	
	int number;
	inode_create(&dummy_inode, &number);
	
	int j, offset;
	for (j = 0; j < 100; j++){
		offset = BLOCK_SIZE * (rand() % 80000);
		offset += rand() % BLOCK_SIZE;

		write_i(number, rand_buf, offset, size);
		write_i(number, zero_buf, offset, size);
	
		read_i(number, actual_result, offset, size);
	
		if (memcmp(zero_buf, actual_result, size) != 0){
			return TEST_FAILED;
		}
	
		inode_read(number, &dummy_inode);
		for (i = 0; i < NUM_DIRECT; i++){
			if (dummy_inode.direct_blocks[i] != 0){
				return TEST_FAILED;
			}
		}
	
		if (dummy_inode.indirect != 0){
			free(disk);
			return TEST_FAILED;
		}
		if (dummy_inode.double_indirect != 0){
			free(disk);
			return TEST_FAILED;
		}
		if (dummy_inode.triple_indirect != 0){
			free(disk);
			return TEST_FAILED;
		}
	}
	
	free(disk);
	return TEST_PASSED;
}

/* PURPOSE:
 *   - Confirm that writing 0s to a file fully deletes it and makes room available for other filesystem
 *   - Confirm that two adjacent double indirect blocks will share their chain
 * METHODOLOGY:
 *   - Write 2 blocks in a double indirect zone, delete them, re-write two blocks nearby
 * EXPECTED RESULTS:
 *   - FS data block usage will never exceed 4 blocks at any one time
 */
int write_on_small_fs_1(){
	printf("%30s", "WRITES_ON_SMALL_FS_1");
	fflush(stdout);
	
	mkfs(MIN_BLOCKS + 4, 0, 0);
	inode dummy_inode;
	memset(&dummy_inode, 0, sizeof(inode));
	
	int number, ret;
	inode_create(&dummy_inode, &number);
	int size = BLOCK_SIZE * 10;

	uint8_t data_buf[size];
	memset(data_buf, 1, size);
	
	uint8_t zero_buf[size];
	memset(zero_buf, 0, size);

	ret = write_i(number, data_buf, BLOCK_SIZE * (ADDRESSES_PER_BLOCK + NUM_DIRECT + 4), BLOCK_SIZE * 2);
	
	ret = write_i(number, zero_buf, BLOCK_SIZE * (ADDRESSES_PER_BLOCK + NUM_DIRECT + 4), BLOCK_SIZE * 2);
	//ret = truncate(number, 0);
	
	ret = write_i(number, data_buf, BLOCK_SIZE * (ADDRESSES_PER_BLOCK + NUM_DIRECT + 8), BLOCK_SIZE * 2);
	
	free(disk);
	if (ret == BLOCK_SIZE * 2){
		return TEST_PASSED;
	}
	
	return TEST_FAILED;
}