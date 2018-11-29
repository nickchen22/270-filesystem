#ifndef LAYER2_H
#define LAYER2_H

#define MAX_FILENAME 256

#define DIRENTS_PER_BLOCK (BLOCK_SIZE / sizeof(dir_ent))
#define DIRENTS_REMAINDER (BLOCK_SIZE % sizeof(dir_ent))

typedef struct __attribute__((__packed__)) dir_ent {
	char name[MAX_FILENAME];
	int inode_num;
} dir_ent;

typedef struct __attribute__((__packed__)) dirblock {
	dir_ent dir_ents[DIRENTS_PER_BLOCK];
	
	uint8_t padding[DIRENTS_REMAINDER];
} dirblock;

int mkdir(const char *pathname, mode_t mode);
int mknod(const char *pathname, mode_t mode, dev_t dev);

#endif