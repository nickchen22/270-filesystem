#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"

/* Initializes a filesystem for use by other functions by doing the following:
 * - Allocates min(MAX_FS_SIZE, blocks) * BLOCK_SIZE bytes to the filesystem
 * - Updates global variables to point to the filesystem
 * - Initializes superblock and sizes of each part of the filesystem (fractional blocks are
 *   assigned to the inodes)
 * - Creates the ilist and ibitmap
 * - Initializes the free list
 * - Creates the root inode and updates the superblock
 *
 * Returns:
 *   TOTALBLOCKS_INVALID - blocks is a negative number
 *   FS_TOO_SMALL        - blocks is smaller than MIN_BLOCKS
 *   BLOCKSIZE_TOO_SMALL - can't fit an inode into a single block
 *   BAD_UID             - provided uid is negative
 *   UNEXPECTED_ERROR    - malloc error
 *   SUCCESS             - filesystem was created
 */
int mkfs(int blocks, int root_uid, int root_gid){
	if (blocks < 0){
		ERR(fprintf(stderr, "ERR: mkfs: blocks is invalid\n"));
		ERR(fprintf(stderr, "  blocks: %d\n", blocks));
		return TOTALBLOCKS_INVALID;
	}

	if (blocks < MIN_BLOCKS){
		ERR(fprintf(stderr, "ERR: mkfs: not enough room for a filesystem\n"));
		ERR(fprintf(stderr, "  blocks: %d\n", blocks));
		return FS_TOO_SMALL;
	}
	
	if (sizeof(inode) > BLOCK_SIZE){
		ERR(fprintf(stderr, "ERR: mkfs: can't fit an inode on a single block\n"));
		ERR(fprintf(stderr, "  sizeof(inode): %d\n", sizeof(inode)));
		ERR(fprintf(stderr, "  BLOCK_SIZE:    %d\n", BLOCK_SIZE));
		return BLOCKSIZE_TOO_SMALL;
	}
	
	if (root_uid < 0 || root_gid < 0){
		ERR(fprintf(stderr, "ERR: mkfs: invalid uid or gid\n"));
		ERR(fprintf(stderr, "  root_uid: %d\n", root_uid));
		ERR(fprintf(stderr, "  root_gid: %d\n", root_gid));
		return BAD_UID;
	}

	/************************ CREATE IN-MEMORY DISK ************************/
	/* TODO: Add support for FS too big */
	/* No point in checking how much RAM is available when we integrate with FUSE.
	 * In the meantime, hardcode restriction to ~1 GB filesystem.
	 * Linux command df tells us how much storage space is available
	 */
	if (blocks > MAX_FS_SIZE){
		blocks = MAX_FS_SIZE;
	}
	disk = malloc(blocks * BLOCK_SIZE);
	total_blocks = blocks;
	
	if (disk == NULL){
		ERR(perror(NULL));
		return UNEXPECTED_ERROR;
	}
	
	DEBUG(DB_MKFS, printf("DEBUG: mkfs: created disk\n"));
	DEBUG(DB_MKFS, printf("  disk:         %p\n", disk));
	DEBUG(DB_MKFS, printf("  total_blocks: %d\n", total_blocks));
	/************************ CREATE IN-MEMORY DISK ************************/
	
	/* Initialize the superblock */
	init_superblock(blocks);
	
	/* Initialize the free list */
	init_freelist();
	
	/* Initialize the ibitmap to 0s */
	init_ibitmap();
	
	/* Create the root inode */
	int root_inode = 0;
	create_dir_base(&root_inode, S_IFDIR | S_IRWXU | S_IRGRP | S_IROTH, root_uid, root_gid, INVALID_INODE);
	
	superblock sb;
	read_superblock(&sb);
	sb.root_inode = root_inode;
	DEBUG(DB_MKFS, printf("  root inode: %d\n", root_inode));
	write_superblock(&sb);

	return SUCCESS;
}

/* Creates a blank directory, which only contains the . and .. items
 *
 * If parent_inum is 0, the parent is set to itself (used in creating root inode)
 *
 * This function is called during mkfs and mkdir and is not really
 * intended for normal use
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   DATA_FULL          - no room for the data block
 *   ILIST_FULL         - no room for the inode
 *   SUCCESS            - superblock was created
 */
int create_dir_base(int* inode_num, mode_t mode, int uid, int gid, int parent_inum){
	int ret;
	
	/* Get an inode */
	int my_inode = 0;
	inode new_dir;
	ret = inode_create(&new_dir, &my_inode);
	if (ret != SUCCESS){
		DEBUG(DB_MKDIRBASE, printf("DEBUG: create_dir_base: couldn't allocate inode\n"));
		return ret;
	}
	
	/* Get and populate a data block */
	int data_num = 0;
	dirblock d;

	dir_ent dot;
	strcpy(dot.name, ".");
	dot.inode_num = my_inode;
	
	dir_ent dot_dot;
	strcpy(dot_dot.name, "..");
	if (parent_inum == INVALID_INODE){
		dot_dot.inode_num = my_inode;
	}
	else{
		dot_dot.inode_num = parent_inum;
	}
	
	d.dir_ents[0] = dot;
	d.dir_ents[1] = dot_dot;
	ret = data_allocate(&d, &data_num);
	if (ret != SUCCESS){
		DEBUG(DB_MKDIRBASE, printf("DEBUG: create_dir_base: couldn't allocate datablock\n"));
		return ret;
	}
	
	/* Initialize the inode */
	memset(&new_dir, 0, sizeof(inode));
	new_dir.mode = mode;
	new_dir.uid = uid;
	new_dir.gid = gid;
	new_dir.size = 2 * sizeof(dir_ent); /* Should never be more than one block, or we have a problem */
	new_dir.direct_blocks[0] = data_num;
	inode_write(my_inode, &new_dir);
	
	DEBUG(DB_MKDIRBASE, printf("DEBUG: create_dir_base: created the dir\n"));
	DEBUG(DB_MKDIRBASE, printf("  my_inode: %d\n", my_inode));
	DEBUG(DB_MKDIRBASE, printf("  data_num: %d\n", data_num));
	
	*inode_num = my_inode;
	return SUCCESS;
}

/* Initializes the superblock given a valid number of blocks for
 * the filesystem to be created on
 *
 * This function is called during mkfs and is not really
 * intended for normal use. It requires that blocks be a valid
 * input
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   SUCCESS            - superblock was created
 */
int init_superblock(int blocks){

	/* Summon math demons to calculate sizes of each part of the filesystem for us */
	int total_iblocks = MAX(MIN_IBITMAP + MIN_INODES, (int)ceil(blocks * INODES_PERCENT));
	int data_blocks = blocks - SUPERBLOCK_SIZE - total_iblocks;

	double inode_blocks_per_bitmap_block = (double)BITS_PER_BLOCK / INODES_PER_BLOCK;
	
	int ibitmap_blocks = (int)ceil(total_iblocks / (inode_blocks_per_bitmap_block + 1));
	int inode_blocks = total_iblocks - ibitmap_blocks;
	
	DEBUG(DB_MKFS, printf("DEBUG: mkfs: doing superblock calculations\n"));
	DEBUG(DB_MKFS, printf("  BLOCK_SIZE:                    %d\n", BLOCK_SIZE));
	DEBUG(DB_MKFS, printf("  SUPERBLOCK_SIZE:               %d\n", SUPERBLOCK_SIZE));
	DEBUG(DB_MKFS, printf("  INODES_PER_BLOCK:              %d\n", INODES_PER_BLOCK));
	DEBUG(DB_MKFS, printf("  sizeof(inode):                 %d\n", sizeof(inode)));
	DEBUG(DB_MKFS, printf("  sizeof(superblock):            %d\n", sizeof(superblock)));
	DEBUG(DB_MKFS, printf("  total_iblocks:                 %d\n", total_iblocks));
	DEBUG(DB_MKFS, printf("  data_blocks:                   %d\n", data_blocks));
	DEBUG(DB_MKFS, printf("  inode_blocks_per_bitmap_block: %f\n", inode_blocks_per_bitmap_block));
	DEBUG(DB_MKFS, printf("  ibitmap_blocks:                %d\n", ibitmap_blocks));
	DEBUG(DB_MKFS, printf("  inode_blocks:                  %d\n", inode_blocks));
	
	/* Initialize fields of superblock */
	superblock sb;
	sb.ibitmap_block_offset = SUPERBLOCK_SIZE;
	sb.ibitmap_size = ibitmap_blocks;
	
	sb.ilist_block_offset = SUPERBLOCK_SIZE + ibitmap_blocks;
	sb.ilist_size = inode_blocks;
	
	sb.data_block_offset = SUPERBLOCK_SIZE + ibitmap_blocks + inode_blocks;
	sb.data_size = data_blocks;
	
	sb.total_blocks = blocks;
	sb.total_inodes = inode_blocks * INODES_PER_BLOCK;
	
	sb.free_list_head = ROOT_FREELIST;
	sb.root_inode = ROOT_INODE;
	
	sb.block_size = BLOCK_SIZE;
	sb.inodes_per_block = INODES_PER_BLOCK;
	sb.num_inodes = inode_blocks * INODES_PER_BLOCK;

	return write_superblock(&sb);
}

/* Initializes the freelist into the data blocks, given their length
 *
 * This function is called during mkfs and is not really
 * intended for normal use. It requires that the filesystem has
 * at least a valid superblock
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   SUCCESS            - freelist was created
 */
int init_freelist(){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: create_freelist: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	int num_data_blocks = sb.data_size;
	freelist_node cur;
	int node_loc, next_loc, i, j;
	
	int blocks_per_node = ADDR_PER_NODE + 1;
	int num_nodes = (int)ceil((double)num_data_blocks / blocks_per_node);
	
	DEBUG(DB_FREELIST, printf("DEBUG: create_freelist: about to begin\n"));
	DEBUG(DB_FREELIST, printf("  num_data_blocks: %d\n", num_data_blocks));
	DEBUG(DB_FREELIST, printf("  blocks_per_node: %d\n", blocks_per_node));
	DEBUG(DB_FREELIST, printf("  num_nodes:       %d\n\n", num_nodes));
	
	/* Create a node and populate it */
	for (i = 0; i < num_nodes; i++){
		node_loc = (i * blocks_per_node) + 1;
		memset(&cur, INVALID_DATA, sizeof(freelist_node));
		
		DEBUG(DB_FREELIST, printf("Building node at location: %d\n", node_loc));
		
		/* Populate it with its free blocks */
		for (j = 0; j < ADDR_PER_NODE && node_loc + j + 1 <= num_data_blocks; j++){
			cur.addr[j] = node_loc + j + 1;
			
			DEBUG(DB_FREELIST, printf("  addr[%d]: %d\n", j, node_loc + j + 1));
		}
		
		/* If there's room for another node after it, link it to the next one */
		next_loc = ((i + 1) * blocks_per_node) + 1; //TODO: make sure this works for weird blocksizes
		if (next_loc <= num_data_blocks){
			cur.next = next_loc;
		}
		
		DEBUG(DB_FREELIST, printf("  next_loc: %d\n", cur.next));
		
		/* Write the node */
		if (data_write(node_loc, &cur) != SUCCESS){
			return DISC_UNINITIALIZED;
		}
	}
	
	return SUCCESS;
}

/* Initializes the ibitmap to all 0s
 * 
 * This function is called during mkfs and is not really
 * intended for normal use. It requires that the filesystem has
 * at least a valid superblock
 * 
 * Returns:
 * 	 DISC_UNINITIALIZED - FS hasn't been set up yet
 *   SUCCESS            - bitmap was initialized
 */
int init_ibitmap(){
	DEBUG(DB_MKFS, printf("DEBUG: init_ibitmap: about to begin\n"));
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: init_ibitmap: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	uint8_t zero_buffer[BLOCK_SIZE];
	memset(zero_buffer, 0, sizeof(zero_buffer));
	
	int cur_ibitmap_block = 0;
	for (cur_ibitmap_block = 0; cur_ibitmap_block < sb.ibitmap_size; cur_ibitmap_block++){
		writeBlock(sb.ibitmap_block_offset + cur_ibitmap_block, zero_buffer);
	}
	
	DEBUG(DB_MKFS, printf("DEBUG: init_ibitmap: ibitmap initialized\n"));
	
	return SUCCESS;
}

/* Reads the specified inode into readNode, a buffer of size sizeof(inode)
 *
 * Note that inodes are 1-indexed. 1 is the first inode,
 * and 0 is not a valid inode
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   BAD_INODE          - not a valid inode in this fs
 *   BUF_NULL           - readNode is null
 *   SUCCESS            - block was read
 */
int inode_read(int inode_num, inode* readNode){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: inode_read: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (readNode == NULL){
		ERR(fprintf(stderr, "ERR: inode_read: readNode is null\n"));
		ERR(fprintf(stderr, "  readNode: %p\n", readNode));
		return BUF_NULL;
	}
	
	if (inode_num <= 0 || inode_num > sb.num_inodes){
		ERR(fprintf(stderr, "ERR: inode_read: inode_num invalid\n"));
		ERR(fprintf(stderr, "  inode_num:       %d\n", inode_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.num_inodes));
		return BAD_INODE;
	}
	
	int inode_block_num = (inode_num - 1) / INODES_PER_BLOCK;
	int inode_in_block = (inode_num - 1) % INODES_PER_BLOCK;

	int total_block_offset = sb.ilist_block_offset + inode_block_num;
	
	iblock block;
	readBlock(total_block_offset, &block);

	DEBUG(DB_INODEREAD, printf("DEBUG: inode_read: reading an inode\n"));
	DEBUG(DB_INODEREAD, printf("  inode_num:                     %d\n", inode_num));
	DEBUG(DB_INODEREAD, printf("  INODES_PER_BLOCK:              %d\n", INODES_PER_BLOCK));
	DEBUG(DB_INODEREAD, printf("  inode_block_num:               %d\n", inode_block_num));
	DEBUG(DB_INODEREAD, printf("  inode_in_block:                %d\n", inode_in_block));
	DEBUG(DB_INODEREAD, printf("  total_block_offset:            %d\n", total_block_offset));
	DEBUG(DB_INODEREAD, printf("  &block:                        %p\n", &block));
	DEBUG(DB_INODEREAD, printf("  &block.inodes[inode_in_block]: %p\n", &block.inodes[inode_in_block]));
	
	memcpy(readNode, &block.inodes[inode_in_block], sizeof(inode));
	
	return SUCCESS;
}

/* Writes modified, a buffer of size sizeof(inode), into the inode located
 * in position inode_num
 *
 * Note that inodes are 1-indexed. 1 is the first inode,
 * and 0 is not a valid inode
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   BAD_INODE          - not a valid inode in this fs
 *   BUF_NULL           - writeNode is null
 *   SUCCESS            - block was written
 */
int inode_write(int inode_num, inode* modified){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: inode_write: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (modified == NULL){
		ERR(fprintf(stderr, "ERR: inode_write: modified is null\n"));
		ERR(fprintf(stderr, "  modified: %p\n", modified));
		return BUF_NULL;
	}
	
	if (inode_num <= 0 || inode_num > sb.num_inodes){
		ERR(fprintf(stderr, "ERR: inode_write: inode_num invalid\n"));
		ERR(fprintf(stderr, "  inode_num:       %d\n", inode_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.num_inodes));
		return BAD_INODE;
	}
	
	int inode_block_num = (inode_num - 1) / INODES_PER_BLOCK;
	int inode_in_block = (inode_num - 1) % INODES_PER_BLOCK;

	int total_block_offset = sb.ilist_block_offset + inode_block_num;
	
	iblock block;
	readBlock(total_block_offset, &block);

	DEBUG(DB_INODEWRITE, printf("DEBUG: inode_write: writing an inode\n"));
	DEBUG(DB_INODEWRITE, printf("  inode_num:                     %d\n", inode_num));
	DEBUG(DB_INODEWRITE, printf("  INODES_PER_BLOCK:              %d\n", INODES_PER_BLOCK));
	DEBUG(DB_INODEWRITE, printf("  inode_block_num:               %d\n", inode_block_num));
	DEBUG(DB_INODEWRITE, printf("  inode_in_block:                %d\n", inode_in_block));
	DEBUG(DB_INODEWRITE, printf("  total_block_offset:            %d\n", total_block_offset));
	DEBUG(DB_INODEWRITE, printf("  &block:                        %p\n", &block));
	DEBUG(DB_INODEWRITE, printf("  &block.inodes[inode_in_block]: %p\n", &block.inodes[inode_in_block]));
	
	memcpy(&block.inodes[inode_in_block], modified, sizeof(inode));
	
	return writeBlock(total_block_offset, &block);
}

/* Marks an inode as free in the ibitmap
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   BAD_INODE      - not a valid inode in our fs
 *   SUCCESS            - block was read
 */
int inode_free(int inode_num){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: inode_write: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (inode_num <= 0 || inode_num > sb.num_inodes){
		ERR(fprintf(stderr, "ERR: inode_free: inode_num invalid\n"));
		ERR(fprintf(stderr, "  inode_num:       %d\n", inode_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.num_inodes));
		return BAD_INODE;
	}
	
	int bitmap_block_num = (inode_num - 1) / BITS_PER_BLOCK;
	int bit_in_block = (inode_num - 1) % BITS_PER_BLOCK;
	
	int byte_in_block = bit_in_block / 8;
	int bit_in_byte = bit_in_block % 8;
	
	int total_block_offset = sb.ibitmap_block_offset + bitmap_block_num;
	
	uint8_t buf[BLOCK_SIZE];
	readBlock(total_block_offset, buf);
	
	DEBUG(DB_INODEFREE, printf("DEBUG: inode_free: setting bit to 0\n"));
	DEBUG(DB_INODEFREE, printf("  inode_num:                 %d\n", inode_num));
	DEBUG(DB_INODEFREE, printf("  INODES_PER_BLOCK:          %d\n", INODES_PER_BLOCK));
	DEBUG(DB_INODEFREE, printf("  bitmap_block_num:          %d\n", bitmap_block_num));
	DEBUG(DB_INODEFREE, printf("  bit_in_block:              %d\n", bit_in_block));
	DEBUG(DB_INODEFREE, printf("  byte_in_block:             %d\n", byte_in_block));
	DEBUG(DB_INODEFREE, printf("  bit_in_byte:               %d\n", bit_in_byte));
	DEBUG(DB_INODEFREE, printf("  total_block_offset:        %d\n", total_block_offset));
	DEBUG(DB_INODEFREE, printf("  buf[byte_in_block] before: %x\n", buf[byte_in_block]));

	/* Clear the bit */
	buf[byte_in_block] &= buf[byte_in_block] ^ (0x80 >> bit_in_byte);
	
	DEBUG(DB_INODEFREE, printf("  buf[byte_in_block] after:  %x\n", buf[byte_in_block]));
	
	return writeBlock(total_block_offset, buf);
}

/* Creates a new inode at the first available location in the ilist
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   BUF_NULL           - newNode is null
 *   INT_NULL           - inode_num is null
 *   ILIST_FULL         - the ilist is full
 *   SUCCESS            - block was read
 */
int inode_create(inode* newNode, int* inode_num){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: inode_create: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	DEBUG(DB_INODECREATE, printf("DEBUG: inode_create: about to begin\n"));
	if (newNode == NULL){
		ERR(fprintf(stderr, "ERR: inode_create: newNode is null\n"));
		ERR(fprintf(stderr, "  newNode: %p\n", newNode));
		return BUF_NULL;
	}
	
	if (inode_num == NULL){
		ERR(fprintf(stderr, "ERR: inode_create: inode_num is null\n"));
		ERR(fprintf(stderr, "  inode_num: %p\n", inode_num));
		return INT_NULL;
	}
	
	uint8_t ibitmap_buffer[BLOCK_SIZE];
	int cur_ibitmap_block;
	int byte_offset, bit_offset = 0;
	int inode_number = 0;
	for (cur_ibitmap_block = 0; cur_ibitmap_block < sb.ibitmap_size; cur_ibitmap_block++){
		/* Read a block of the ibitmap */
		ret = readBlock(sb.ibitmap_block_offset + cur_ibitmap_block, ibitmap_buffer);
		
		DEBUG(DB_INODECREATE, printf("DEBUG: inode_create: read a block of the ibitmap\n"));
		DEBUG(DB_INODECREATE, printf("  cur_ibitmap_block:           %d\n", cur_ibitmap_block));
		
		int free_bit = 0;
		for (byte_offset = 0; byte_offset < sizeof(ibitmap_buffer); byte_offset++){
			uint8_t byte = ibitmap_buffer[byte_offset];
			if (byte != 0xff){ // If a byte is full, continue
				for (bit_offset = 0; bit_offset < 8; bit_offset++){
					/* Calculate the inode_number of this bit */ //TODO this could be sped up but I don't think it matters much
					inode_number = cur_ibitmap_block * (BLOCK_SIZE * 8);
					inode_number += byte_offset * 8;
					inode_number += bit_offset;
					inode_number += 1; // One-indexed
					
					if (inode_number > sb.num_inodes){
						ERR(fprintf(stderr, "ERR: inode_create: ilist is full\n"));
						return ILIST_FULL;
					}
					
					if (((byte << bit_offset) & 0x80) == 0){
						DEBUG(DB_INODECREATE, printf("DEBUG: inode_create: found a free spot\n"));
						DEBUG(DB_INODECREATE, printf("  byte_offset:             %d\n", byte_offset));
						DEBUG(DB_INODECREATE, printf("  bit_offset:              %d\n", bit_offset));
						DEBUG(DB_INODECREATE, printf("  cur_ibitmap_block:       %d\n", cur_ibitmap_block));
						DEBUG(DB_INODECREATE, printf("  sb.ibitmap_block_offset: %d\n", sb.ibitmap_block_offset));
						
						DEBUG(DB_INODECREATE, printf("  byte (before):           %x\n", byte));
						
						/* Update the ibitmap */
						byte |= (0x80 >> bit_offset);
						ibitmap_buffer[byte_offset] = byte;
						writeBlock(sb.ibitmap_block_offset + cur_ibitmap_block, ibitmap_buffer);
						
						DEBUG(DB_INODECREATE, printf("  byte (after):            %x\n", byte));
						DEBUG(DB_INODECREATE, printf("  inode_number:            %d\n", inode_number));
						
						*inode_num = inode_number;
						return inode_write(inode_number, newNode);
					}
				}
			}
		}
	}
	
	ERR(fprintf(stderr, "ERR: inode_create: ilist is full\n"));
	return ILIST_FULL;
}

/* Reads the specified data block into readBuf, a buffer of size BLOCK_SIZE
 *
 * Note that data blocks are 1-indexed. 1 is the first data block,
 * and 0 is not a valid data block
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   INVALID_BLOCK      - not a valid data block in our fs
 *   BUF_NULL           - readBuf is null
 *   SUCCESS            - block was read
 */
int data_read(int data_block_num, void* readBuf){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: data_read: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (data_block_num < 0 || data_block_num > sb.data_size){
		ERR(fprintf(stderr, "ERR: data_read: data_block_num invalid\n"));
		ERR(fprintf(stderr, "  data_block_num:  %d\n", data_block_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.data_size));
		return INVALID_BLOCK;
	}
	
	/* Reading the 0-block returns all 0s */
	if (data_block_num == 0){
		memset(readBuf, 0, BLOCK_SIZE);
		return SUCCESS;
	}
	
	int total_offset = sb.data_block_offset + data_block_num - 1;

	DEBUG(DB_READDATA, printf("DEBUG: data_read: reading data block\n"));
	DEBUG(DB_READDATA, printf("  sb.data_block_offset: %d\n", sb.data_block_offset));
	DEBUG(DB_READDATA, printf("  data_block_num:       %d\n", data_block_num));
	DEBUG(DB_READDATA, printf("  total_offset:         %d\n", total_offset));
	
	return readBlock(total_offset, readBuf);
}

/* Writes writeBuf, a buffer of size BLOCK_SIZE, to the specified data block
 *
 * Note that data blocks are 1-indexed. 1 is the first data block,
 * and 0 is not a valid data block
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   INVALID_BLOCK      - not a valid data block in our fs
 *   BUF_NULL           - writeBuf is null
 *   SUCCESS            - block was written
 */
int data_write(int data_block_num, void* writeBuf){ //TODO: fix camelCase + disk vs disc
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: data_write: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (data_block_num <= 0 || data_block_num > sb.data_size){
		ERR(fprintf(stderr, "ERR: data_write: data_block_num invalid\n"));
		ERR(fprintf(stderr, "  data_block_num:  %d\n", data_block_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.data_size));
		return INVALID_BLOCK;
	}
	
	int total_offset = sb.data_block_offset + data_block_num - 1;

	DEBUG(DB_WRITEDATA, printf("DEBUG: data_write: writing data block\n"));
	DEBUG(DB_WRITEDATA, printf("  sb.data_block_offset: %d\n", sb.data_block_offset));
	DEBUG(DB_WRITEDATA, printf("  data_block_num:       %d\n", data_block_num));
	DEBUG(DB_WRITEDATA, printf("  total_offset:         %d\n", total_offset));
	
	return writeBlock(total_offset, writeBuf);
}

/* Puts a data block on the free list. Does not do any error checking
 *
 * Note that data blocks are 1-indexed. 1 is the first data block,
 * and 0 is not a valid data block
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   INVALID_BLOCK      - not a valid data block in our fs
 *   SUCCESS            - block was added to free list
 */
int data_free(int data_block_num){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: data_free: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (data_block_num <= 0 || data_block_num > sb.data_size){
		ERR(fprintf(stderr, "ERR: data_free: data_block_num invalid\n"));
		ERR(fprintf(stderr, "  data_block_num:  %d\n", data_block_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.data_size));
		return INVALID_BLOCK;
	}
	
	int original_root = sb.free_list_head;

	int cur_loc = original_root;
	freelist_node cur_node;
	
	DEBUG(DB_DATAFREE, printf("DEBUG: data_free: scanning freelist for spot\n"));
	DEBUG(DB_DATAFREE, printf("  cur_loc: %d\n", cur_loc));
	
	/* Scan the free list until the end */
	int i;
	while(cur_loc != INVALID_DATA){
		/* Read the free list node */
		data_read(cur_loc, &cur_node);
		
		/* Scan it for a spot to put our free'd block on */
		for (i = 0; i < ADDR_PER_NODE; i++){
			if (cur_node.addr[i] == INVALID_DATA){
				DEBUG(DB_DATAFREE, printf("DEBUG: data_free: found a spot\n"));
				DEBUG(DB_DATAFREE, printf("  cur_loc:          %d\n", cur_loc));
				DEBUG(DB_DATAFREE, printf("  i:                %d\n", i));
				
				cur_node.addr[i] = data_block_num;
				data_write(cur_loc, &cur_node);
				return SUCCESS;
			}
		}
		
		/* If it's full, go to the next one */
		cur_loc = cur_node.next;
		
		DEBUG(DB_DATAFREE, printf("  cur_loc: %d\n", cur_loc));
	}
	
	/* Hit the end of the free list without finding a spot to put our
	 * newly free'd node. Thus, we're going to transform the current block
	 * into a new node for the freelist:
	 */
	memset(&cur_node, INVALID_DATA, sizeof(freelist_node));

	/* It becomes the new root */
	sb.free_list_head = data_block_num;
	cur_node.next = original_root;
	
	DEBUG(DB_DATAFREE, printf("DEBUG: data_free: couldn't find a spot, creating new head\n"));
	DEBUG(DB_DATAFREE, printf("  sb.free_list_head: %d\n", sb.free_list_head));
	DEBUG(DB_DATAFREE, printf("  cur_node.next:     %d\n", cur_node.next));

	/* Write the changes to disk */
	data_write(data_block_num, &cur_node);
	write_superblock(&sb);

	return SUCCESS;
}

/* Finds a data block from the freelist and initializes it to
 * newData. Puts the data block found in data_block_num
 *
 * Note that data blocks are 1-indexed. 1 is the first data block,
 * and 0 is not a valid data block
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   DATA_FULL          - filesystem is full
 *   SUCCESS            - a block was found and returned
 */
int data_allocate(void* newData, int* data_block_num){
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: data_allocate: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (sb.free_list_head == INVALID_DATA){
		ERR(fprintf(stderr, "ERR: data_allocate: data blocks are full\n"));
		return DATA_FULL;
	}
	
	/* Look at the first node of the freelist */
	freelist_node cur;
	data_read(sb.free_list_head, &cur);
	
	DEBUG(DB_DATAALL, printf("DEBUG: data_allocate: checking freelist head\n"));
	DEBUG(DB_DATAALL, printf("  sb.free_list_head: %d\n", sb.free_list_head));
	
	/* Scan its addresses */
	int i;
	*data_block_num = sb.free_list_head;
	for (i = 0; i < ADDR_PER_NODE; i++){
		/* If a free node is found, give the block to the user and update the node */
		if (cur.addr[i] != INVALID_DATA){
			*data_block_num = cur.addr[i];
			cur.addr[i] = INVALID_DATA;
			data_write(sb.free_list_head, &cur);
			
			DEBUG(DB_DATAALL, printf("  i:                 %d\n", i));
			DEBUG(DB_DATAALL, printf("  *data_block_num:   %d\n", *data_block_num));
			
			return data_write(*data_block_num, newData);
		}
	}
	
	/* Otherwise, give the user the head of the free list and update sb */
	sb.free_list_head = cur.next;
	write_superblock(&sb);
	
	DEBUG(DB_DATAALL, printf("DEBUG: data_allocate: giving user the old freelist head\n"));
	DEBUG(DB_DATAALL, printf("  sb.free_list_head: %d\n", sb.free_list_head));
	DEBUG(DB_DATAALL, printf("  *data_block_num:   %d\n", *data_block_num));
	
	return data_write(*data_block_num, newData);
}

/* Reads the superblock into the provided object, if it exists
 *
 * This function assumes the disk hasn't been messed with. If you manually set
 * disk instead of calling mkfs or leaving disk NULL, this isn't guaranteed to work right
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - superblock buffer is null
 *   SUCCESS            - read the superblock
 */
int read_superblock(superblock* sb){
	if (sb == NULL){
		ERR(fprintf(stderr, "ERR: read_superblock: sb is null\n"));
		ERR(fprintf(stderr, "  sb: %p\n", sb));
		return BUF_NULL;
	}
	
	uint8_t temp_buffer[BLOCK_SIZE * SUPERBLOCK_SIZE];
	
	/* Read the superblock into temp_buffer, one block at a time */
	int i, ret;
	for (i = 0; i < SUPERBLOCK_SIZE; i++){
		/* Read a block and check the return value */
		ret = readBlock(i, &temp_buffer[BLOCK_SIZE * i]);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: read_superblock: readBlock failed\n"));
			ERR(fprintf(stderr, "  ret: %d\n", ret));
			return DISC_UNINITIALIZED;
		}
		
		DEBUG(DB_READSB, printf("DEBUG: read_superblock: read a block of the superblock\n"));
		DEBUG(DB_READSB, printf("  i:           %d\n", i));
		DEBUG(DB_READSB, printf("  write_addr:  %p\n", &temp_buffer[BLOCK_SIZE * i]));
		DEBUG(DB_READSB, printf("  temp_buffer: %p\n", temp_buffer));
	}
	
	memcpy(sb, temp_buffer, sizeof(superblock));

	return SUCCESS;
}

/* Writes the provided superblock to disk
 *
 * This function assumes the disk hasn't been messed with. If you manually set
 * disk instead of calling mkfs or leaving disk NULL, this isn't guaranteed to work right
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - superblock buffer is null
 *   SUCCESS            - read the superblock
 */
int write_superblock(superblock* sb){
	if (sb == NULL){
		ERR(fprintf(stderr, "ERR: write_superblock: sb is null\n"));
		ERR(fprintf(stderr, "  sb: %p\n", sb));
		return BUF_NULL;
	}
	
	/* Put the suberblock into a block-size buffer */
	uint8_t temp_buffer[BLOCK_SIZE * SUPERBLOCK_SIZE];
	memcpy(temp_buffer, sb, sizeof(superblock));
	
	/* Write the superblock to disk, one block at a time */
	int i, ret;
	for (i = 0; i < SUPERBLOCK_SIZE; i++){
		/* Write a block and check the return value */
		ret = writeBlock(i, &temp_buffer[BLOCK_SIZE * i]);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: write_superblock: writeBlock failed\n"));
			ERR(fprintf(stderr, "  ret: %d\n", ret));
			return DISC_UNINITIALIZED;
		}
		
		DEBUG(DB_WRITESB, printf("DEBUG: write_superblock: wrote a block of the superblock\n"));
		DEBUG(DB_WRITESB, printf("  i:           %d\n", i));
		DEBUG(DB_WRITESB, printf("  write_addr:  %p\n", &temp_buffer[BLOCK_SIZE * i]));
		DEBUG(DB_WRITESB, printf("  temp_buffer: %p\n", temp_buffer));
	}

	return SUCCESS;
}