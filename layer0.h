#ifndef LAYER0_H
#define LAYER0_H

#include "stdint.h"

static uint8_t* disk
static int total_blocks = -1;
#define BLOCK_SIZE 4096

int writeBlock(int blocknum, uint8_t* writeBuf);
int readBlock(int blocknum, uint8_t* readBuf);

#endif
