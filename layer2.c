#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"

//TODO: not currently bothering with mode, link count, dev, times

/* mkdir() attempts to create a directory named pathname
 *
 * Assumes that the initial directory elements will fit in a block
 *
 * Returns:
 *   DISC_UNINITIALIZED - FS hasn't been set up yet
 *   BAD_INODE          - not a valid inode in this fs
 *   BUF_NULL           - readNode is null
 *   SUCCESS            - block was read
 */
int mkdir(const char *pathname, mode_t mode){
	dir_ent dot;
	strcpy(dot.name, ".");
	dot.inode_num = 3;
	
	dir_ent dotdot;
	
	
	inode new_dir;
	new_dir.mode = S_IFREG & S_IRWXU; // regular file and RWX permissions
	new_dir.links = 0;
	new_dir.owner_id = 0; // owner id corresponds to the inode in the test set for identificatin purposes
	new_dir.size = 200;  // size of the file this inode belongs to is less than one block size
	new_dir.access_time = 0;
	new_dir.mod_time = 0;
	int i;
	new_dir.direct_blocks[0] = 0;
	for (i = 1; i < 10; i ++){
		new_dir.direct_blocks[i] = 0; // assumes addresses with 0 are unused
	}
	new_dir.indirect = 0;
	new_dir.double_indirect = 0;
}