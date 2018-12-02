#ifndef LAYER2_H
#define LAYER2_H

#define FILEBUF_SIZE MAX_FILENAME + 1
#define MAX_FILENAME 256

#define DIRENTS_PER_BLOCK (BLOCK_SIZE / sizeof(dir_ent))
#define DIRENTS_REMAINDER (BLOCK_SIZE % sizeof(dir_ent))

#define ADDRESSES_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))
#define ADDRESSES_REMAINDER (BLOCK_SIZE % sizeof(uint32_t))

typedef struct __attribute__((__packed__)) address_block {
	uint32_t address[ADDRESSES_PER_BLOCK];

	uint8_t padding[ADDRESSES_REMAINDER];
} address_block;

typedef struct __attribute__((__packed__)) dir_ent {
	char name[FILEBUF_SIZE];
	int inode_num;
} dir_ent;

typedef struct __attribute__((__packed__)) dirblock {
	dir_ent dir_ents[DIRENTS_PER_BLOCK];
	
	uint8_t padding[DIRENTS_REMAINDER];
} dirblock;

int mkdir_fs(const char *pathname, mode_t mode, int uid, int gid);
int mknod_fs(const char *pathname, mode_t mode, int uid, int gid);

int check_permissions(int inum, int uid, int gid, int* read, int* write, int* exec);
int namei(const char *pathname, int uid, int gid, int* parent_inum, int* target_inum);

int add_dirent(int inum, dir_ent* d);
int remove_dirent(int inum, int index);
int search_by_name(int inum, const char *name, int* target);
int search_by_inum(int inum, int search_num, int* index);
int read_dir_whole(int inum, dir_ent* buf);
int read_dir_page(int inum, dirblock* d, int page, int* entries, int* last);
int truncate(int inum, off_t offset);
int del(int inum);

int read_i(int inum, void* buf, off_t offset, size_t size);
int write_i(int inum, void* buf, off_t offset, size_t size);

int get_nth_datablock(inode* inod, off_t n, int create, int* created);
int rm_nth_datablock(inode* inod, off_t n);
int intPow(int x, int y);

#endif