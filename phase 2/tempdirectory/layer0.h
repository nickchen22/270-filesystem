#ifndef LAYER0_H
#define LAYER0_H

#define UNINITIALIZED_FD -1
#define NUM_BLOCKS 5000

/* Representation of the disk in core memory */
extern int layer0_fd;

/* Writes the data contained in write_buf,
 * a buffer of size BLOCK_SIZE, into the global
 * disk variable at the blocknum-th block
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
int write_block(int blocknum, void* write_buf);

/* Reads the data located at the blocknum-th block in the
 * global disk variable into a buffer located at read_buf
 * of size BLOCK_SIZE
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
int read_block(int blocknum, void* read_buf);

#endif
