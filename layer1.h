#ifndef LAYER1_H
#define LAYER1_H

/* TODO: cache? superblock in program memory? */
typedef struct __attribute__((__packed__)) superblock {
	uint32_t ibitmap_block_offset;
	uint32_t ibitmap_size; //in blocks
	
	uint32_t ilist_block_offset;
	uint32_t ilist_size; //in blocks

	uint32_t data_block_offset;
	uint32_t data_size;

	uint32_t total_blocks;
	uint32_t total_inodes;
	
	uint32_t free_list_head; /* TODO: Include free list tail? */
	uint32_t root_inode;
	
	uint32_t block_size;
	uint32_t inodes_per_block;
	
	uint8_t padding[0];
} superblock;

typedef struct __attribute__((__packed__)) inode {
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

typedef struct __attribute__((__packed__)) freelist_node {
	uint32_t next;
	uint32_t addr[(BLOCK_SIZE - sizeof(uint32_t)) / sizeof(uint32_t)];
} freelist_node;

int mkfs(int blocks);

int inode_read(int inode_num, inode* readNode);
int inode_write(int inode_num, inode* modified);
int inode_free(int inode_num);
int inode_create(inode* newNode, int* inode_num);

int data_read(int data_block_num, uint8_t* readBuf);
int data_write(int data_block_num, uint8_t* writeBuf);
int date_free(int data_block_num);
int data_allocate(uint8_t* newData, int* data_block_num);

int read_superblock(superblock* sb);

#endif
