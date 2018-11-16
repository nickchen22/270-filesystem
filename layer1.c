#include "globals.h"
#include "layer1.h"
#include "layer0.h"

/* The minimum number of blocks we can possibly make a valid FS on */
#define MIN_IBITMAP 1
#define MIN_INODES 1
#define MIN_DATA 1
#define MIN_BLOCKS (SUPERBLOCK_SIZE + MIN_IBITMAP + MIN_INODES + MIN_DATA)

/* The rough percentage of non-superblock blocks to use for i-nodes */
#define INODES_PERCENT 0.1

/* Size of the superblock, in blocks */
#define SUPERBLOCK_SIZE ((int)ceil(sizeof(superblock) / (double)BLOCK_SIZE))

#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(inode))

#define BITS_PER_BLOCK (BLOCK_SIZE * 8)

#define ROOT_INODE 1
#define ROOT_FREELIST 1

/* 
	Initiliazes filesystem by allocating a buffer of size
	blocks * BLOCK_SIZE. Assigns disk to point to the
	beginning of this buffer. Initializes superblock and
	establishes percentage ratio between data blocks and ilist blocks
	(fractional blocks wil be assigned to the ilist). Creates ilist
	bit map between superblock and ilist with the size defined, in bits,
	as the number of inodes in the ilist. Initializes free list, creates 
	root inode and updates the superblock.
	
	only works if sizeof(inode) <= blocksize

	Returns:
		- Error: negative total blocks
		- Error: not enough blocks for superblock/ilist
		- Adjustment: too many blocks, not enough room on disk
		- Success
*/
int mkfs(int blocks){
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
		ERR(fprintf(stderr, "  BLOCK_SIZE: %d\n", BLOCK_SIZE));
		return BLOCKSIZE_TOO_SMALL;
	}
	
	/************************ CREATE IN-MEMORY DISK ************************/
	/* TODO: Add support for FS too big */

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
	
	/* TODO: have to use writeblock lol whoops */
	superblock* sb = (superblock*)disk;
	sb->ibitmap_block_offset = SUPERBLOCK_SIZE;
	sb->ibitmap_size = ibitmap_blocks;
	
	sb->ilist_block_offset = SUPERBLOCK_SIZE + ibitmap_blocks;
	sb->ilist_size = inode_blocks;
	
	sb->data_block_offset = SUPERBLOCK_SIZE + ibitmap_blocks + inode_blocks;
	sb->data_size = data_blocks;
	
	sb->total_blocks = blocks;
	sb->total_inodes = inode_blocks * INODES_PER_BLOCK;
	
	sb->free_list_head = ROOT_FREELIST;
	sb->root_inode = ROOT_INODE;
	
	sb->block_size = BLOCK_SIZE;
	sb->inodes_per_block = INODES_PER_BLOCK;
	
	/* create root inode, initialize ibitmap */
	
	/* create root directory? */
	
	/* initialize free list */
	
	freelist_node a;
	data_read(1, (uint8_t*)&a);

	return SUCCESS;
}

/*
	Reads the inode from the index inode_num in the ilist and
	passes it by reference to the address readNode.

	Returns:
		- Error: FS uninitialized
		- readNode is null
		- inode_num out of range
		- inode_num not allocated?
*/
int inode_read(int inode_num, inode* readNode) {

}

/*
	Writes an inode to the index inode_num in the ilist and
	passes the new inode to the address modified.

	Returns:
		- FS uninitialized
		- modified is null
		- inode_num out of range
*/
int inode_write(int inode_num, inode* modified) {

}

/*
	Marks inode as free in both the inode structure and ilist bit
	map.

	Returns:
		- FS uninitialized
		- inode_num is out of range
		- inode is already free
*/
int inode_free(int inode_num) {

}

/*
	Creates a new inode at the first available place in the ilist.
	Passes the newly created inode to the address newNode and its
	corresponding index in the ilist to the address inode_num.

	Returns:
		- FS uninitialized
		- newNode is null
		- not enough room in ilist
		- inode_num is null
*/
int inode_create(inode* newNode, int* inode_num) {

}

/* Reads the specified data block into readBuf
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
int data_read(int data_block_num, uint8_t* readBuf) {
	superblock sb;

	int ret = read_superblock(&sb);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: data_read: read_superblock failed\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return DISC_UNINITIALIZED;
	}
	
	if (data_block_num <= 0 || data_block_num > sb.data_size){
		ERR(fprintf(stderr, "ERR: data_read: data_block_num invalid\n"));
		ERR(fprintf(stderr, "  data_block_num:  %d\n", data_block_num));
		ERR(fprintf(stderr, "  min (exclusive): %d\n", 0));
		ERR(fprintf(stderr, "  max (inclusive): %d\n", sb.data_size));
		return INVALID_BLOCK;
	}
	
	int total_offset = sb.data_block_offset + data_block_num - 1;

	DEBUG(DB_READDATA, printf("DEBUG: data_read: reading data block\n"));
	DEBUG(DB_READDATA, printf("  sb.data_block_offset: %d\n", sb.data_block_offset));
	DEBUG(DB_READDATA, printf("  data_block_num:       %d\n", data_block_num));
	DEBUG(DB_READDATA, printf("  total_offset:         %d\n", total_offset));
	
	return readBlock(total_offset, readBuf);
}

/* Writes writeBuf to the specified data block
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
int data_write(int data_block_num, uint8_t* writeBuf){ //TODO: it's possible writeBuf should be a void pointer
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

/*
	Frees a data block specified by data_block_num and adds places it
	onto the freelist.

	Returns:
		- FS uninitialized

*/
int date_free(int data_block_num) {

}

/*
	Creates a data block by finding a free block on the free list.

*/
int data_allocate(uint8_t* newData, int* data_block_num) {

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
	
	int i, ret;
	/* Read the superblock into temp_buffer, one block at a time */
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