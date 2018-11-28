#ifndef LAYER1_H
#define LAYER1_H

#define READNODE_NULL -2
#define INODE_NUM_OUT_OF_RANGE -3
/* The minimum number of blocks we can possibly make a valid FS on */
#define MIN_IBITMAP 1
#define MIN_INODES 1
#define MIN_DATA 1
#define MIN_BLOCKS (SUPERBLOCK_SIZE + MIN_IBITMAP + MIN_INODES + MIN_DATA)

/* The rough percentage of non-superblock blocks to use for i-nodes */
#define INODES_PERCENT 0.1

/* Size of the superblock, in blocks */
#define SUPERBLOCK_SIZE ((int)ceil(sizeof(superblock) / (double)BLOCK_SIZE))

/* Dimensions of the freelist nodes */
#define ADDR_PER_NODE ((BLOCK_SIZE - sizeof(uint32_t)) / sizeof(uint32_t))
#define REMAINDER ((BLOCK_SIZE - sizeof(uint32_t)) % sizeof(uint32_t))

/* How inodes are packed into blocks */
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(inode))
#define INODES_REMAINDER (BLOCK_SIZE % sizeof(inode))

#define BITS_PER_BLOCK (BLOCK_SIZE * 8)

#define ROOT_INODE 1
#define ROOT_FREELIST 1
#define INVALID_INODE 0
#define INVALID_DATA 0

/* Max filesystem size in blocks (roughly 1 GB in bytes). To be removed when FUSE is integrated */
#define MAX_FS_SIZE 250000

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
	uint32_t num_inodes;
	
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

typedef struct __attribute__((__packed__)) iblock {
	inode inodes[INODES_PER_BLOCK];
	
	uint8_t padding[INODES_REMAINDER];
} iblock;

typedef struct __attribute__((__packed__)) freelist_node {
	uint32_t next;
	uint32_t addr[ADDR_PER_NODE];
	
	uint8_t padding[REMAINDER];
} freelist_node;

int mkfs(int blocks);
int init_superblock(int blocks);
int init_freelist();
int init_ibitmap();

int inode_read(int inode_num, inode* readNode);
int inode_write(int inode_num, inode* modified);
int inode_free(int inode_num);
int inode_create(inode* newNode, int* inode_num);

int data_read(int data_block_num, uint8_t* readBuf);
int data_write(int data_block_num, uint8_t* writeBuf);
int data_free(int data_block_num);
int data_allocate(uint8_t* newData, int* data_block_num);

int read_superblock(superblock* sb);
int write_superblock(superblock* sb);

#endif
