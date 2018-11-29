#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"
#include <limits.h>

#define TEST_FAILED 0
#define TEST_PASSED 1

int mkfs_size();
int lots_inodes();
void print_inode(inode* inode);

int main(int argc, const char **argv){
	int (*tests[])() = {mkfs_size, lots_inodes};
	int max_test = sizeof(tests) / sizeof(tests[0]);
	int num_tests = MAX(max_test, argc - 1);
	
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

/* Prints an inode */
void print_inode(inode* inode){
	printf("inode->mode:           %d\n", inode->mode);
	printf("inode->links:          %d\n", inode->links);
	printf("inode->owner_id:       %d\n", inode->owner_id);
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
		
		actual_result = mkfs(test_value);
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

	mkfs(BLOCKS);
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