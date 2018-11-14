#include "globals.h"
#include "layer1.h"
#include "layer0.h"

/* 
	Initiliazes filesystem by allocating a buffer of size
	total_blocks * BLOCK_SIZE. Assigns disk to point to the
	beginning of this buffer. Initializes superblock and
	establishes percentage ratio between data blocks and ilist blocks
	(fractional blocks wil be assigned to the ilist). Creates ilist
	bit map between superblock and ilist with the size defined, in bits,
	as the number of inodes in the ilist. Initializes free list, creates 
	root inode and updates the superblock.

	Returns:
		- Error: negative total blocks
		- Error: not enough blocks for superblock/ilist
		- Adjustment: too many blocks, not enough room on disk
		- Success
*/
int mkfs(int totalBlocks) {
	if (totalBlocks <= 1) {
		ERR(fprintf(stderr, "ERR: mkfs: totalBlocks is invalid\n"));
		ERR(fprintf(stderr, "  totalBlocks: %d\n", totalBlocks));
		return TOTALBLOCKS_INVALID;
	}
	total_blocks = totalBlocks;
	disk = (uint8_t*)malloc(totalBlocks * BLOCK_SIZE);
	sb = (superblock*)malloc(sizeof(superblock));
	createSB(sb);

	return SUCCESS;
}

// Create superblock helper function
void createSB(superblock *sb) {
	sb->ilist_block_offset = 1;
	sb->ilist_size = 10;
	sb->data_block_offset = ((int) ceil(sb->ilist_size * sizeof(inode) / BLOCK_SIZE) ) + 1;
	sb->data_size = total_blocks - sb->data_block_offset;
	sb->total_blocks = total_blocks;
	sb->ilist_map = (bool*)malloc(sizeof(bool) * sb->ilist_size);
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

/*
	Reads from a data block specified by data_block_num in reference to
	its offset from the beginning of the datablocks. Places contents read
	into readBuf.

	Returns:
		- FS uninitialized
		- readBuf is null
		- data_block_num is out of bounds
		- data_block_num is not allocated?
*/
int data_read(int data_block_num, uint8_t* readBuf) {

}

/*
	Writes to a data block specified by data_block_num in reference to
	its offset from the beginning of the datablocks. Writes contents
	from writeBuf.

	Returns:
		- FS uninitialized
		- writeBuf is null
		- data_block_num is out of bounds
		- data_block_num is allocated?
*/
int data_write(int data_block_num, uint8_t* writeBuf) {

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
int data_create(uint8_t* newData, int* data_block_num) {

}





