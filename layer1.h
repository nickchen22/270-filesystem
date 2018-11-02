#ifndef "LAYER1_H"
#define "LAYER1_H"
#include "stdint.h"

typedef struct superblock {
	uint32_t ilist_block_offset;
	uint32_t ilist_size;
	uint32_t data_block_offset;
	uint32_t data_size;
	uint32_t total_blocks;
} superblock;

typedef struct inode {
	uint32_t mode;
	uint32_t links;
	uint32_t owner_id;
	uint32_t size;
	uint32_t access_time;
	uint32_t mod_time;
	uint32_t direct_blocks[10];
	uint32_t indirect;
	uint32_t double_indirect;
} inode;

int mkfs(int total_blocks);

int inode_read(int inode_num, uint8_t* buffer);
int inode_write(int inode_num, inode* modified);
int inode_free(int inode_num);
int inode_create(uint_t* buffer);

int data_read();
int data_write();
int date_free();
int data_create();

#endif
