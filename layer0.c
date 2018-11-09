#include "globals.h"
#include "layer0.h"

uint8_t* disk = NULL;
int total_blocks = UNINITIALIZED_BLOCKS;

/* Writes the information contained in writeBuf,
 * a buffer of size BLOCK_SIZE, into the global
 * disk variable at a location specified by blocknum
 *
 * Blocks are 0-indexed, blocknum = 0 will write to the
 * first block
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   WRITEBUF_NULL      - write buffer is null
 *   INVALID_BLOCK      - invalid block specified
 *   SUCCESS            - wrote data to disk
 */
int writeBlock(int blocknum, uint8_t* writeBuf){
	if (total_blocks == UNINITIALIZED_BLOCKS || disk == NULL){
		ERR(fprintf(stderr, "ERR: writeBlock: disk uninitialized\n"));
		ERR(fprintf(stderr, "  disk: %p\n", disk));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return DISC_UNINITIALIZED;
	}
	
	if (blocknum >= total_blocks || blocknum < 0){
		ERR(fprintf(stderr, "ERR: writeBlock: invalid block\n"));
		ERR(fprintf(stderr, "  blocknum: %d\n", blocknum));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return INVALID_BLOCK;
	}
	
	if (writeBuf == NULL){
		ERR(fprintf(stderr, "ERR: writeBlock: writeBuf is null\n"));
		ERR(fprintf(stderr, "  writeBuf: %p\n", writeBuf));
		return WRITEBUF_NULL;
	}
	
	uintptr_t write_start = (uintptr_t)disk + blocknum * BLOCK_SIZE;
	
	DEBUG(DB_WRITEBLOCK, printf("DEBUG: writeBlock:\n"));
	DEBUG(DB_WRITEBLOCK, printf("  disk:        %p\n", disk));
	DEBUG(DB_WRITEBLOCK, printf("  blocknum:    %d\n", blocknum));
	DEBUG(DB_WRITEBLOCK, printf("  blocksize:   %d\n", BLOCK_SIZE));
	DEBUG(DB_WRITEBLOCK, printf("  write_start: %p\n", (void *)write_start));
	DEBUG(DB_WRITEBLOCK, printf("  writeBuf:    %p\n", writeBuf));
	
	memcpy((void*)write_start, writeBuf, BLOCK_SIZE);
	
	return SUCCESS;
}

/* Reads the data contained in the block specified by blocknum
 * from the global disk space and places it in the buffer readBuf
 *
 * Blocks are 0-indexed, blocknum = 0 will read the
 * first block
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   READBUF_NULL       - read buffer is null
 *   INVALID_BLOCK      - invalid block specified
 *   SUCCESS            - read data to buffer
 */

int readBlock(int blocknum, uint8_t* readBuf){
	if (total_blocks == UNINITIALIZED_BLOCKS || disk == NULL){
		ERR(fprintf(stderr, "ERR: readBlock: disk uninitialized\n"));
		ERR(fprintf(stderr, "  disk: %p\n", disk));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return DISC_UNINITIALIZED;
	}
	
	if (blocknum >= total_blocks || blocknum < 0){
		ERR(fprintf(stderr, "ERR: readBlock: invalid block\n"));
		ERR(fprintf(stderr, "  blocknum: %d\n", blocknum));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return INVALID_BLOCK;
	}
	
	if (readBuf == NULL){
		ERR(fprintf(stderr, "ERR: readBlock: readBuf is null\n"));
		ERR(fprintf(stderr, "  readBuf: %p\n", readBuf));
		return READBUF_NULL;
	}
	DEBUG(DB_READBLOCK, printf("%d\n", total_blocks));
	

	return SUCCESS;
}