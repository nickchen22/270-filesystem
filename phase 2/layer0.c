#include "globals.h"
#include "layer0.h"

int layer0_fd = UNINITIALIZED_FD;

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
int write_block(int blocknum, void* write_buf){
	if (layer0_fd == UNINITIALIZED_FD){
		ERR(fprintf(stderr, "ERR: write_block: disk uninitialized\n"));
		ERR(fprintf(stderr, "  layer0_fd: %d\n", layer0_fd));
		return DISC_UNINITIALIZED;
	}
	
	if (blocknum >= NUM_BLOCKS || blocknum < 0){
		ERR(fprintf(stderr, "ERR: write_block: invalid block\n"));
		ERR(fprintf(stderr, "  blocknum:     %d\n", blocknum));
		ERR(fprintf(stderr, "  NUM_BLOCKS:   %d\n", NUM_BLOCKS));
		return INVALID_BLOCK;
	}

	if (write_buf == NULL){
		ERR(fprintf(stderr, "ERR: write_block: write_buf is null\n"));
		ERR(fprintf(stderr, "  write_buf: %p\n", write_buf));
		return BUF_NULL;
	}
	
	off_t write_start = (off_t)blocknum * BLOCK_SIZE;
	
	DEBUG(DB_WRITEBLOCK, printf("DEBUG: write_block: about to write\n"));
	DEBUG(DB_WRITEBLOCK, printf("  layer0_fd:   %d\n", layer0_fd));
	DEBUG(DB_WRITEBLOCK, printf("  blocknum:    %d\n", blocknum));
	DEBUG(DB_WRITEBLOCK, printf("  blocksize:   %d\n", BLOCK_SIZE));
	DEBUG(DB_WRITEBLOCK, printf("  write_start: %p\n", (void *)write_start));
	DEBUG(DB_WRITEBLOCK, printf("  write_buf:   %p\n", write_buf));
	
	lseek(layer0_fd, write_start, SEEK_SET);
	write(layer0_fd, write_buf, BLOCK_SIZE);
	
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
int read_block(int blocknum, void* read_buf){
	if (layer0_fd == UNINITIALIZED_FD){
		ERR(fprintf(stderr, "ERR: read_block: disk uninitialized\n"));
		ERR(fprintf(stderr, "  layer0_fd: %d\n", layer0_fd));
		return DISC_UNINITIALIZED;
	}
	
	if (blocknum >= NUM_BLOCKS || blocknum < 0){
		ERR(fprintf(stderr, "ERR: read_block: invalid block\n"));
		ERR(fprintf(stderr, "  blocknum:     %d\n", blocknum));
		ERR(fprintf(stderr, "  NUM_BLOCKS:   %d\n", NUM_BLOCKS));
		return INVALID_BLOCK;
	}
	
	if (read_buf == NULL){
		ERR(fprintf(stderr, "ERR: read_block: read_buf is null\n"));
		ERR(fprintf(stderr, "  read_buf: %p\n", read_buf));
		return BUF_NULL;
	}
	
	off_t read_start = (off_t)blocknum * BLOCK_SIZE;
	
	DEBUG(DB_READBLOCK, printf("DEBUG: read_block: about to read\n"));
	DEBUG(DB_READBLOCK, printf("  layer0_fd:   %d\n", layer0_fd));
	DEBUG(DB_READBLOCK, printf("  blocknum:    %d\n", blocknum));
	DEBUG(DB_READBLOCK, printf("  blocksize:   %d\n", BLOCK_SIZE));
	DEBUG(DB_READBLOCK, printf("  read_start:  %p\n", (void *)read_start));
	DEBUG(DB_READBLOCK, printf("  read_buf:    %p\n", read_buf));
	
	lseek(layer0_fd, read_start, SEEK_SET);
	read(layer0_fd, read_buf, BLOCK_SIZE);

	return SUCCESS;
}