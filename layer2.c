#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"

//TODO: not currently bothering with link count, dev, times

/* mkdir() attempts to create a directory named pathname
 *
 * Assumes that the initial directory elements will fit in a block
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   BAD_INODE          - not a valid inode in this fs
 *   BUF_NULL           - readNode is null
 *   SUCCESS            - block was read
 */
int mkdir(const char *pathname, mode_t mode){ // mkdir - create a default directory
	//1. run namei on the base path to check for rwx permissions
	//2. create a default dir object
	//3. add default dir object to directory found in step 1 with name from step 1


	dir_ent dot;
	strcpy(dot.name, ".");
	dot.inode_num = 3;
	
	dir_ent dotdot;
	
	
	inode new_dir;
	new_dir.mode = S_IFREG & S_IRWXU; // regular file and RWX permissions
	new_dir.links = 0;
	new_dir.size = 200;  // size of the file this inode belongs to is less than one block size
	new_dir.access_time = 0;
	new_dir.mod_time = 0;
	int i;
	new_dir.direct_blocks[0] = 0;
	for (i = 1; i < 10; i ++){
		new_dir.direct_blocks[i] = 0; // assumes addresses with 0 are unused
	}
	new_dir.indirect = 0;
	new_dir.double_indirect = 0;
}

//mknod - create a default inode

//add to directory - gets current size, writes new entry at that offset?
//remove from directory - when you delete something, it moves the thing at the end to that spot?
//search directory

// all permissions checking done here
int namei(const char *pathname, int uid, int gid, int must_exist){
	return SUCCESS;
}

//unlink - consults open file table about removal of a file
int unlink_i(int inum){
	return SUCCESS;
}

//open - checks permissions | add to open file table
//close/release - if an entry is pending for deletetion in the open file table, delete it. otherwise decrease reference count?

//readdir - calls read????

/* Reads size data from inum at offset offset into buf
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - read buffer is null
 *   INVALID_BLOCK      - a data block found was invalid
 *   INT                - upon success, returns number of bytes read
 */
int read_i(int inum, void* buf, int offset, size_t size){
	if (buf == NULL){
		ERR(fprintf(stderr, "ERR: read_i: buf is null\n"));
		ERR(fprintf(stderr, "  buf: %p\n", buf));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_i: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		return ret;
	}

	/* If the write position is beyond the end of the file */
	if (offset >= my_inode.size){
		return 0;
	}
	
	/* Consider the length we will read */
	int truncated_size = MIN(my_inode.size - offset, size);
	if (truncated_size == 0){
		return 0;
	}
	
	/* Calculate reading start and end */
	int start_block = offset / BLOCK_SIZE;
	int start_offset = offset % BLOCK_SIZE;

	int end_block = (offset + truncated_size - 1) / BLOCK_SIZE; //not sure this is correct
	int end_size = ((offset + truncated_size - 1) % BLOCK_SIZE) + 1; //1-4096
	
	DEBUG(DB_READI, printf("DEBUG: read_i: calculated offsets\n"));
	DEBUG(DB_READI, printf("  start_block:         %d\n", start_block));
	DEBUG(DB_READI, printf("  start_offset:        %d\n", start_offset));
	DEBUG(DB_READI, printf("  end_block:           %d\n", end_block));
	DEBUG(DB_READI, printf("  end_size:            %d\n", end_size));

	/* Read the first partial block. Special case if it's also the last block */
	int block_addr;
	uint8_t block_buf[BLOCK_SIZE];
	uintptr_t writeOffset = 0;
	
	block_addr = get_nth_datablock(&my_inode, start_block, FALSE, NULL);
	DEBUG(DB_READI, printf("  block_addr (%06d): %d\n", start_block, block_addr));
	
	ret = data_read(block_addr, &block_buf);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_i: data_read failed\n"));
		ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
		return ret;
	}
	
	if (start_block == end_block){
		memcpy(buf, &block_buf[start_offset], end_size - start_offset);
		return end_size - start_offset;
	}
	else{
		memcpy(buf, &block_buf[start_offset], BLOCK_SIZE - start_offset);
		writeOffset += (BLOCK_SIZE - start_offset);
	}
	
	/* Read intermediate full blocks */
	int i;
	for (i = start_block + 1; i < end_block; i++){
		block_addr = get_nth_datablock(&my_inode, i, FALSE, NULL);
		DEBUG(DB_READI, printf("  block_addr (%06d): %d\n", i, block_addr));
		
		data_read(block_addr, &block_buf);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: read_i: data_read failed\n"));
			ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
			return ret;
		}
		
		memcpy((void*)((uintptr_t)buf + (uintptr_t)writeOffset), block_buf, BLOCK_SIZE);
		writeOffset += BLOCK_SIZE;
	}
	
	/* Read the last, partial block */
	block_addr = get_nth_datablock(&my_inode, end_block, FALSE, NULL);
	DEBUG(DB_READI, printf("  block_addr (%06d): %d\n", end_block, block_addr));
	
	data_read(block_addr, &block_buf);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_i: data_read failed\n"));
		ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
		return ret;
	}
		
	memcpy((void*)((uintptr_t)buf + (uintptr_t)writeOffset), block_buf, end_size);
	
	return truncated_size;
}

//  fuse wrapper will consult open file table to find inode?
//write - walk the file, writing as much as we can, allocating as we go. update inode at each step. if a block is all 0s we call rm instead
//      - for partial blocks, will need to call get and then put, unless we're extending the file in some cases?
//      - clear isuid and isgid
int write_i(int inum, void* buf, int offset, int size){
	return SUCCESS;
}

// should be able to handle dangling indirect blocks - ex if I allocate a triple indirect block but then run out of space before I can put anything in it
int rm_nth_datablock(){
}

/* Returns the data block number associated with the nth block of a file
 *
 * If create is true, get_nth_datablock will attempt to create the datablock, if
 * it doesn't exist. Doing this may update inod and data blocks in the filesystem.
 * If a data block is created, created will be set to TRUE, otherwise it is unchanged
 *
 * Note that n is zero-indexed. n = 0 will return the first block of the file
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - inode is null
 *   DATA_FULL          - if create is true and the block (or an intermediate block) cannot be created
 *   INVALID_BLOCK      - an invalid block number was found during the search
 *   INT                - upon success, returns number for data block
 */
int get_nth_datablock(inode* inod, int n, int create, int* created){
	if (n < 0){
		ERR(fprintf(stderr, "ERR: get_nth_datablock: block num was negative\n"));
		ERR(fprintf(stderr, "  n: %d\n", n));
		return INVALID_BLOCK;
	}
	
	if (inod == NULL){
		ERR(fprintf(stderr, "ERR: get_nth_datablock: inode is null\n"));
		ERR(fprintf(stderr, "  inod: %p\n", inod));
		return BUF_NULL;
	}
	
	int num_direct = NUM_DIRECT; //10
	int single_indirect = num_direct + ADDRESSES_PER_BLOCK; //10 + 1024
	int double_indirect = single_indirect + ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK; //10 + 1024 + 1048576
	int triple_indirect = double_indirect + ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK;
	
	address_block b;
	int offset, next_addr, next_index, ret, new_block;
	int* from_pointer;
	
	uint8_t empty_block[BLOCK_SIZE];
	memset(empty_block, 0, BLOCK_SIZE);
	
	DEBUG(DB_GETNTH, printf("DEBUG: get_nth_datablock: about to begin\n"));
	DEBUG(DB_GETNTH, printf("  n:                %d\n", n));
	DEBUG(DB_GETNTH, printf("  num_direct:       %d\n", num_direct));
	DEBUG(DB_GETNTH, printf("  single_indirect:  %d\n", single_indirect));
	DEBUG(DB_GETNTH, printf("  double_indirect:  %d\n", double_indirect));
	DEBUG(DB_GETNTH, printf("  triple_indirect:  %d\n", triple_indirect));
	DEBUG(DB_GETNTH, printf("  inod:             %p\n", inod));
	DEBUG(DB_GETNTH, printf("  &inod->indirect:  %p\n", &inod->indirect));
	
	/* Figure out what type of block our target is, update i and next_addr */
	offset = n;
	int i;
	if (offset < num_direct){
		i = -1;
		next_addr = inod->direct_blocks[offset];
		from_pointer = &inod->direct_blocks[offset];
	}
	else if (offset < single_indirect){
		offset -= num_direct;
		i = 0;
		next_addr = inod->indirect;
		from_pointer = &inod->indirect;
	}
	else if (offset < double_indirect){
		offset -= single_indirect;
		i = 1;
		next_addr = inod->double_indirect;
		from_pointer = &inod->double_indirect;
	}
	else if (offset < triple_indirect){
		offset -= double_indirect;
		i = 2;
		next_addr = inod->triple_indirect;
		from_pointer = &inod->triple_indirect;
	}
	else{
		ERR(fprintf(stderr, "ERR: get_nth_datablock: block num was too big\n"));
		ERR(fprintf(stderr, "  triple_indirect: %d\n", triple_indirect));
		ERR(fprintf(stderr, "  n:               %d\n", n));
		return INVALID_BLOCK;
	}
	
	DEBUG(DB_GETNTH, printf("  next_addr:        %d\n", next_addr));

	/* Special case when creating a first-layer block, since we have to update the inode */
	if (next_addr == 0 && create){
		/* Create a new block of all zeros */
		ret = data_allocate(&empty_block, &new_block);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: get_nth_datablock: filesystem full\n"));
			ERR(fprintf(stderr, "  ret: %d\n", ret));
			return ret;
		}

		DEBUG(DB_GETNTH, printf("  DEBUG: get_nth_datablock: created a new block\n"));
		DEBUG(DB_GETNTH, printf("    new_block: %d\n", new_block));
		
		/* Update values to point to new block */
		*from_pointer = new_block;
		next_addr = new_block;
		
		/* Do not update the inode itself, that's the caller's job */
		*created = TRUE;
	}
	
	/* Continue down indirect blocks */
	for (; i >= 0; i--){
		if (next_addr == 0 && !create) return 0;
		ret = data_read(next_addr, &b);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: get_nth_datablock: data_read failed\n"));
			ERR(fprintf(stderr, "  next_addr:            %d\n", next_addr));
			ERR(fprintf(stderr, "  ret:                  %d\n", ret));
			return ret;
		}
		
		next_index = offset / intPow(ADDRESSES_PER_BLOCK, i);
		offset = offset % intPow(ADDRESSES_PER_BLOCK, i);
		
		DEBUG(DB_GETNTH, printf("DEBUG: get_nth_datablock: doing an iteration\n"));
		DEBUG(DB_GETNTH, printf("  i:          %d\n", i));
		DEBUG(DB_GETNTH, printf("  next_index: %d\n", next_index));
		DEBUG(DB_GETNTH, printf("  offset:     %d\n", offset));
		
		/* If the next block needs to be created */
		if (b.address[next_index] == 0 && create){
			/* Create a new block of all zeros */
			ret = data_allocate(&empty_block, &new_block);
			if (ret != SUCCESS){
				ERR(fprintf(stderr, "ERR: get_nth_datablock: filesystem full\n"));
				ERR(fprintf(stderr, "  ret: %d\n", ret));
				return ret;
			}

			DEBUG(DB_GETNTH, printf("  DEBUG: get_nth_datablock: created a new block\n"));
			DEBUG(DB_GETNTH, printf("    new_block: %d\n", new_block));
			
			/* Update b and write it back */
			b.address[next_index] = new_block;
			ret = data_write(next_addr, &b);
			if (ret != SUCCESS){
				ERR(fprintf(stderr, "ERR: get_nth_datablock: tried to write an invalid block\n"));
				ERR(fprintf(stderr, "  next_addr: %d\n", next_addr));
				ERR(fprintf(stderr, "  ret:       %d\n", ret));
				return ret;
			}
			*created = TRUE;
		}
		
		next_addr = b.address[next_index];
		
		DEBUG(DB_GETNTH, printf("  next_addr:  %d\n", next_addr));
	}
	
	return next_addr;
 }
 
/* Simple helper function to calculate pow for positive ints */
int intPow(int x, int y){
	int sum = 1;
	int i;
	for (i = 0; i < y; i++){
		sum *= x;
	}

	return sum;
}

//TODO: rmdir?

//Work plan:
// 1. read_inode, write_inode
// 2. readdir
// 3. add to dir, remove from dir, search dir, truncate/delete?
// 4. namei
// 5. mknod mkdir
// 6. open file table
// 7. open, close, unlink



//Open file table consists of two parts:
// 1. for each open inode, has number of references, parent inode, pending deletion flag
// 2. for each fd, has a inode and the type that it was opened with (O_APPND, O_WRONLY, etc)

// no writeback caching