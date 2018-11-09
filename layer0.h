#ifndef LAYER0_H
#define LAYER0_H

#define UNINITIALIZED_BLOCKS -1

#define DISC_UNINITIALIZED -1
#define INVALID_BLOCK -2
#define WRITEBUF_NULL -3

extern uint8_t* disk;
extern int total_blocks;

int writeBlock(int blocknum, uint8_t* writeBuf);
int readBlock(int blocknum, uint8_t* readBuf);

#endif
