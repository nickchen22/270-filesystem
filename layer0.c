#include "globals.h"
#include "layer0.h"

uint8_t* disk = NULL;
int total_blocks = UNINITIALIZED_BLOCKS;

/* Writes the data contained in write_buf,
 * a buffer of size BLOCK_SIZE, into the global
 * disk variable at the blocknum-th block
 *
 * Blocks are 0-indexed, blocknum = 0 will write to the
 * first block
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - write buffer is null
 *   INVALID_BLOCK      - invalid block specified
 *   SUCCESS            - wrote data to disk
 */
int writeBlock(int blocknum, uint8_t* write_buf){
	if (total_blocks == UNINITIALIZED_BLOCKS || disk == NULL){
		ERR(fprintf(stderr, "ERR: writeBlock: disk uninitialized\n"));
		ERR(fprintf(stderr, "  disk:         %p\n", disk));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return DISC_UNINITIALIZED;
	}
	
	if (blocknum >= total_blocks || blocknum < 0){
		ERR(fprintf(stderr, "ERR: writeBlock: invalid block\n"));
		ERR(fprintf(stderr, "  blocknum:     %d\n", blocknum));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return INVALID_BLOCK;
	}
	
	if (write_buf == NULL){
		ERR(fprintf(stderr, "ERR: writeBlock: write_buf is null\n"));
		ERR(fprintf(stderr, "  write_buf: %p\n", write_buf));
		return BUF_NULL;
	}
	
	uintptr_t write_start = (uintptr_t)disk + blocknum * BLOCK_SIZE;
	
	DEBUG(DB_WRITEBLOCK, printf("DEBUG: writeBlock: about to write\n"));
	DEBUG(DB_WRITEBLOCK, printf("  disk:        %p\n", disk));
	DEBUG(DB_WRITEBLOCK, printf("  blocknum:    %d\n", blocknum));
	DEBUG(DB_WRITEBLOCK, printf("  blocksize:   %d\n", BLOCK_SIZE));
	DEBUG(DB_WRITEBLOCK, printf("  write_start: %p\n", (void *)write_start));
	DEBUG(DB_WRITEBLOCK, printf("  write_buf:   %p\n", write_buf));
	
	memcpy((void*)write_start, write_buf, BLOCK_SIZE);
	
	return SUCCESS;
}

/* Reads the data located at the blocknum-th block in the
 * global disk variable into a buffer located at read_buf
 * of size BLOCK_SIZE
 *
 * Blocks are 0-indexed, blocknum = 0 will read the
 * first block
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - read buffer is null
 *   INVALID_BLOCK      - invalid block specified
 *   SUCCESS            - read data to buffer
 */
int readBlock(int blocknum, uint8_t* read_buf){
	if (total_blocks == UNINITIALIZED_BLOCKS || disk == NULL){
		ERR(fprintf(stderr, "ERR: readBlock: disk uninitialized\n"));
		ERR(fprintf(stderr, "  disk:         %p\n", disk));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return DISC_UNINITIALIZED;
	}
	
	if (blocknum >= total_blocks || blocknum < 0){
		ERR(fprintf(stderr, "ERR: readBlock: invalid block\n"));
		ERR(fprintf(stderr, "  blocknum:     %d\n", blocknum));
		ERR(fprintf(stderr, "  total_blocks: %d\n", total_blocks));
		return INVALID_BLOCK;
	}
	
	if (read_buf == NULL){
		ERR(fprintf(stderr, "ERR: readBlock: read_buf is null\n"));
		ERR(fprintf(stderr, "  read_buf: %p\n", read_buf));
		return BUF_NULL;
	}
	
	uintptr_t read_start = (uintptr_t)disk + blocknum * BLOCK_SIZE;
	
	DEBUG(DB_READBLOCK, printf("DEBUG: readBlock: about to read\n"));
	DEBUG(DB_READBLOCK, printf("  disk:        %p\n", disk));
	DEBUG(DB_READBLOCK, printf("  blocknum:    %d\n", blocknum));
	DEBUG(DB_READBLOCK, printf("  blocksize:   %d\n", BLOCK_SIZE));
	DEBUG(DB_READBLOCK, printf("  read_start:  %p\n", (void *)read_start));
	DEBUG(DB_READBLOCK, printf("  read_buf:    %p\n", read_buf));
	
	memcpy(read_buf, (void*)read_start, BLOCK_SIZE);

	return SUCCESS;
}