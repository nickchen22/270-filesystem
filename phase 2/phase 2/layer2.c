#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"

//TODO: not currently bothering with link count, dev, times
//TODO: assuming file paths do not contain ., .., or a / at the end
//TODO: make sure writeback caching disabled

/* Table 1 of OFT */
oft_inode* oft_inodes = NULL;
int oft_inodes_size = 0;

/* Table 2 of OFT */
oft_fd* oft_fds = NULL;
int oft_fds_size = 0;

/* Adds an item to the OFT
 *
 * Returns:
 *   fd - returns the fd for this item in the OFT
 */
int oft_add(int inode, int flags){
	/* Add to table 1 */
	oft_inode n;
	int already_exists = FALSE;
	int i, fd;
	for (i = 0; i < oft_inodes_size; i++){
		n = oft_inodes[i];
		if (n.inum == inode){
			n.ref++;
			memcpy(&oft_inodes[i], &n, sizeof(oft_inode));
			already_exists = TRUE;
			break;
		}
	}
	
	if (! already_exists){
		oft_inodes_size++;
		oft_inodes = realloc(oft_inodes, oft_inodes_size * sizeof(oft_inode));
		n.inum = inode;
		n.ref = 1;
		n.pending_deletion = FALSE;
		memcpy(&oft_inodes[oft_inodes_size - 1], &n, sizeof(oft_inode));
	}
	
	/* Add to table 2 */
	oft_fd f;
	int fd_good;
	for (fd = MIN_FD;; fd++){
		fd_good = TRUE;
		for (i = 0; i < oft_fds_size; i++){
			f = oft_fds[i];
			if (f.fd == fd){
				fd_good = FALSE;
				break;
			}
		}
		
		if (fd_good){
			break;
		}
	}
	
	oft_fds_size++;
	oft_fds = realloc(oft_fds, oft_fds_size * sizeof(oft_fd));
	f.fd = fd;
	f.inum = inode;
	f.flags = flags;
	memcpy(&oft_fds[oft_fds_size - 1], &f, sizeof(oft_fd));
	
	return fd;
}

/* Removes an item from the OFT. If it is pending deletion and
 * the last reference, it will be deleted.
 *
 * Returns:
 *   TRUE  - fd was removed from both OFT entries
 *   FALSE - fd couldn't be found in an OFT entry
 */
int oft_remove(int fd){
	/* Remove from table 2 */
	oft_fd f;
	int inode = -1;
	int i;
	for (i = 0; i < oft_fds_size; i++){
		f = oft_fds[i];
		if (f.fd == fd){
			inode = f.inum;
			
			/* Remove from table by moving rest back + calling realloc */
			if (i + 1 < oft_fds_size){
				memmove(&oft_fds[i], &oft_fds[i + 1], sizeof(oft_fd) * (oft_fds_size - i - 1));
			}
			oft_fds_size--;
			oft_fds = realloc(oft_fds, oft_fds_size * sizeof(oft_fd));
		}
	}
	
	if (inode == -1){
		ERR(fprintf(stderr, "ERR: oft_remove: requested fd not found\n"));
		ERR(fprintf(stderr, "  fd: %d\n", fd));
		return FALSE;
	}
	
	/* Remove from table 1 */
	oft_inode n;
	for (i = 0; i < oft_inodes_size; i++){
		n = oft_inodes[i];
		if (n.inum == inode){
			n.ref--;
			if (n.ref == 0){
				if (n.pending_deletion == TRUE){
					del(inode);
				}
				/* Remove from table by moving rest back + calling realloc */
				if (i + 1 < oft_inodes_size){
					memmove(&oft_inodes[i], &oft_inodes[i + 1], sizeof(oft_inode) * (oft_inodes_size - i - 1));
				}
				oft_inodes_size--;
				oft_inodes = realloc(oft_inodes, oft_inodes_size * sizeof(oft_inode));
				return TRUE;
			}
			memcpy(&oft_inodes[i], &n, sizeof(oft_inode));
			return TRUE;
		}
	}
	
	return FALSE;
}

/* Tries to delete the contents of a file if it isn't
 * currently in use. If it is, sets the pending deletion flag
 *
 * Returns:
 *   TRUE  - inode was deleted
 *   FALSE - inode is currently open and is now pending deletion
 */
int oft_attempt_delete(int inode){
	oft_inode n;
	int i;
	for (i = 0; i < oft_inodes_size; i++){
		n = oft_inodes[i];
		if (n.inum == inode){
			n.pending_deletion = TRUE;
			memcpy(&oft_inodes[i], &n, sizeof(oft_inode));
			return FALSE;
		}
	}
	
	del(inode);
	return TRUE;
}

/* Searches the OFT for the entry associated with fd
 *
 * Sets inode and flags if it is found
 *
 * Returns:
 *   TRUE  - fd was found
 *   FALSE - fd was not found in the open file table
 */
int oft_lookup(int fd, int* inode, int* flags){
	oft_fd f;
	int i;
	for (i = 0; i < oft_fds_size; i++){
		f = oft_fds[i];
		if (f.fd == fd){
			*inode = f.inum;
			*flags = f.flags;

			return TRUE;
		}
	}

	return FALSE;
}

/* Returns the stat structure associated with an inode
 *
 * On inode read failure, returns an empty stat structure
 */
struct stat get_stat(int inum){
	struct stat s;
	memset(&s, 0, sizeof(stat));
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: get_stat: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return s;
	}
	
	s.st_ino = inum;
	s.st_mode = my_inode.mode;
	s.st_uid = my_inode.uid;
	s.st_gid = my_inode.gid;
	s.st_size = my_inode.size;
	
	return s;
}

/* mkdir() attempts to create a directory named pathname
 *
 * Assumes that the initial directory elements will fit in a block
 *
 * Returns:
 *   -EACCES       - a component of the path didn't have rx permissions
 *   -ENOENT       - a component of the path didn't exist
 *   -ENOTDIR      - a component of the path wasn't a directory
 *   -ENAMETOOLONG - a component of the path had a name longer than 256 bytes
 *   -ENOSPC       - no room in FS for the new file or directory entry
 *   SUCCESS       - directory was created
 */
int mkdir_fs(const char *pathname, mode_t mode, int uid, int gid){
	/* Lookup path */
	int parent_inum, target_inum, ret, index;
	if ((ret = namei(pathname, uid, gid, &parent_inum, &target_inum, &index)) != SUCCESS){
		ERR(fprintf(stderr, "ERR: mkdir_fs: error occured during lookup\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return ret;
	}
	
	/* Check the parent node for rwx permissions */
	int read, write, exec;
	check_permissions(parent_inum, uid, gid, &read, &write, &exec);
	if (!read || !exec || !write){
		ERR(fprintf(stderr, "ERR: mkdir_fs: parent didn't have rwx permissions\n"));
		return -EACCES;
	}

	/* Check that target doesn't already exist */
	if (target_inum != 0){
		ERR(fprintf(stderr, "ERR: mkdir_fs: target already exists\n"));
		return -EEXIST;
	}

	/* Create a default directory object, if there's rooom */
	int new_inum;
	if ((ret = create_dir_base(&new_inum, mode, uid, gid, parent_inum)) != SUCCESS){
		ERR(fprintf(stderr, "ERR: mkdir_fs: no space for new directory\n"));
		return -ENOSPC;
	}

	/* Relies on assumption that no provided filepath will end in / */
	char* target_name = strrchr(pathname, '/') + 1;
	dir_ent new;
	new.inode_num = new_inum;
	strncpy(new.name, target_name, FILEBUF_SIZE);
	
	/* Add the new entry to the parent directory */
	if (add_dirent(parent_inum, &new) != sizeof(dir_ent)){
		ERR(fprintf(stderr, "ERR: mkdir_fs: no space for new entry in old directory\n"));
		del(new_inum);
		return -ENOSPC;
	}

	return SUCCESS;
}

/* mknod() attempts to create a file named pathname
 *
 * Assumes that the initial directory elements will fit in a block
 *
 * Returns:
 *   -EACCES       - a component of the path didn't have rx permissions
 *   -ENOENT       - a component of the path didn't exist
 *   -ENOTDIR      - a component of the path wasn't a directory
 *   -ENAMETOOLONG - a component of the path had a name longer than 256 bytes
 *   -ENOSPC       - no room in FS for the new file or directory entry
 *   SUCCESS       - file was created
 */
int mknod_fs(const char *pathname, mode_t mode, int uid, int gid){
	/* Lookup path */
	int parent_inum, target_inum, ret, index;
	if ((ret = namei(pathname, uid, gid, &parent_inum, &target_inum, &index)) != SUCCESS){
		ERR(fprintf(stderr, "ERR: mknod_fs: error occured during lookup\n"));
		ERR(fprintf(stderr, "  ret: %d\n", ret));
		return ret;
	}
	
	/* Check the parent node for rwx permissions */
	int read, write, exec;
	check_permissions(parent_inum, uid, gid, &read, &write, &exec);
	if (!read || !exec || !write){
		ERR(fprintf(stderr, "ERR: mknod_fs: parent didn't have rwx permissions\n"));
		return -EACCES;
	}

	/* Check that target doesn't already exist */
	if (target_inum != 0){
		ERR(fprintf(stderr, "ERR: mknod_fs: target already exists\n"));
		return -EEXIST;
	}

	/* Create a default inode object, if there's rooom */
	int new_inum;
	inode new_inode;
	memset(&new_inode, 0, sizeof(inode));
	new_inode.mode = S_IFREG | mode;
	new_inode.mode &= (0xffff ^ (S_IFDIR));
	new_inode.uid = uid;
	new_inode.gid = gid;
	if ((ret = inode_create(&new_inode, &new_inum)) != SUCCESS){
		ERR(fprintf(stderr, "ERR: mknod_fs: no space for new directory\n"));
		return -ENOSPC;
	}

	/* Relies on assumption that no provided filepath will end in / */
	char* target_name = strrchr(pathname, '/') + 1;
	dir_ent new;
	new.inode_num = new_inum;
	strncpy(new.name, target_name, FILEBUF_SIZE);
	
	/* Add the new entry to the parent directory */
	if (add_dirent(parent_inum, &new) != sizeof(dir_ent)){
		ERR(fprintf(stderr, "ERR: mknod_fs: no space for new entry in old directory\n"));
		del(new_inum);
		return -ENOSPC;
	}

	return SUCCESS;
}

/* Converts a pathname into a parent inum and a target inum. If the target inum
 * doesn't currently exist, it will be set to 0
 *
 * Returns:
 *   -EACCES       - a component of the path didn't have rx permissions
 *   -ENOENT       - a component of the path didn't exist
 *   -ENOTDIR      - a component of the path wasn't a directory
 *   -ENAMETOOLONG - a component of the path had a name longer than 256 bytes
 *   SUCCESS       - the pathname was converted into parent_inum and target_inum
 */
int namei(const char *pathname, int uid, int gid, int* parent_inum, int* target_inum, int* index){
	char* path_copy = strdup(pathname);
	const char delim[2] = "/";
	
	int read, write, exec;
	*parent_inum = ROOT_INODE;
	*target_inum = ROOT_INODE;
	*index = 0;
	int ret;
	
	char* token;

	token = strtok(path_copy, delim);
	while (token != NULL){ //there's more
		/* Evaluate the old target */
		*parent_inum = *target_inum;
		if (*parent_inum == 0){
			free(path_copy);
			ERR(fprintf(stderr, "ERR: namei: couldn't find a path component\n"));
			return -ENOENT;
		}
		if (strlen(token) > MAX_FILENAME){
			free(path_copy);
			ERR(fprintf(stderr, "ERR: namei: component's name is too long\n"));
			return -ENAMETOOLONG;
		}

		/* Check the parent node for search permissions */
		check_permissions(*parent_inum, uid, gid, &read, &write, &exec);
		if (!read || !exec){
			free(path_copy);
			ERR(fprintf(stderr, "ERR: namei: component didn't have rx permissions\n"));
			return -EACCES;
		}

		/* Look for the new target */
		ret = search_by_name(*parent_inum, token, target_inum, index);
		if (ret == NOT_DIR){
			free(path_copy);
			ERR(fprintf(stderr, "ERR: namei: error occured while parsing component\n"));
			return -ENOTDIR;
		}
		else if (ret == NOT_IN_DIR){
			*target_inum = 0;
		}

		token = strtok(NULL, delim);
	}
	
	free(path_copy);
	return SUCCESS;
}

/* Checks to see if a file can be read/written/exected by a uid + gid
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - an input was null
 *   SUCCESS            - read, write, exec were set
 */
int check_permissions(int inum, int uid, int gid, int* read, int* write, int* exec){
	if (read == NULL || write == NULL || exec == NULL){
		ERR(fprintf(stderr, "ERR: check_permissions: something is null\n"));
		ERR(fprintf(stderr, "  read:   %p\n", read));
		ERR(fprintf(stderr, "  write:  %p\n", write));
		ERR(fprintf(stderr, "  exec:   %p\n", exec));
		return BUF_NULL;
	}
	
	*read = FALSE;
	*write = FALSE;
	*exec = FALSE;

	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: check_permissions: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}

	/* Check for owner */
	if (my_inode.uid == uid){
		if ((S_IRUSR & my_inode.mode) != 0){
			*read = TRUE;
		}
		if ((S_IWUSR & my_inode.mode) != 0){
			*write = TRUE;
		}
		if ((S_IXUSR & my_inode.mode) != 0){
			*exec = TRUE;
		}
	}

	/* Check for group members */
	if (my_inode.gid == gid){
		if ((S_IRGRP & my_inode.mode) != 0){
			*read = TRUE;
		}
		if ((S_IWGRP & my_inode.mode) != 0){
			*write = TRUE;
		}
		if ((S_IXGRP & my_inode.mode) != 0){
			*exec = TRUE;
		}
	}

	/* Check for others */
	if (my_inode.uid != uid && my_inode.gid != gid){
		if ((S_IROTH & my_inode.mode) != 0){
			*read = TRUE;
		}
		if ((S_IWOTH & my_inode.mode) != 0){
			*write = TRUE;
		}
		if ((S_IXOTH & my_inode.mode) != 0){
			*exec = TRUE;
		}
	}
	
	return SUCCESS;
}

/* Searches the directory at inum to find an entry matching search_num
 *
 * If the entry is found, target is set to its index in the directory (which can change)
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - given input pointer was null
 *   NOT_DIR            - error occured while parsing the directory
 *   NOT_IN_DIR         - something with the name wasn't found
 *   SUCCESS            - was found, target was set
 */
int search_by_inum(int inum, int search_num, int* index){
	if (index == NULL){
		ERR(fprintf(stderr, "ERR: search_by_inum: index is null\n"));
		ERR(fprintf(stderr, "  index:   %p\n", index));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: search_by_inum: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	if (! S_ISDIR(my_inode.mode)){
		ERR(fprintf(stderr, "ERR: search_by_inum: not a directory\n"));
		ERR(fprintf(stderr, "  my_inode.mode:           %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  S_ISDIR(my_inode.mode):  %d\n", S_ISDIR(my_inode.mode)));
		return NOT_DIR;
	}
	
	int cur_block = 0;
	int reached_end = FALSE;
	int num_entries, i;
	dirblock d;
	for (cur_block = 0; reached_end == FALSE; cur_block++){
		if (read_dir_page(inum, &d, cur_block, &num_entries, &reached_end) == SUCCESS){
			for (i = 0; i < num_entries; i++){
				if (d.dir_ents[i].inode_num == search_num){
					*index = cur_block * DIRENTS_PER_BLOCK + i;
					return SUCCESS;
				}
			}
		}
		else{
			ERR(fprintf(stderr, "ERR: search_by_inum: read_dir failed\n"));
			return NOT_DIR;
		}
	}
	
	return NOT_IN_DIR;
}

/* Searches the directory at inum to find an entry matching name
 *
 * If the entry is found, target is set to its inode number and index is set to
 * its 0-indexed location in the parent directory
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - given input pointer was null
 *   NOT_DIR            - error occured while parsing the directory
 *   NOT_IN_DIR         - something with the name wasn't found
 *   SUCCESS            - was found, target was set
 */
int search_by_name(int inum, const char *name, int* target, int* index){
	if (name == NULL || target == NULL){
		ERR(fprintf(stderr, "ERR: search_by_name: something is null\n"));
		ERR(fprintf(stderr, "  name:   %p\n", name));
		ERR(fprintf(stderr, "  target: %p\n", target));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: search_by_name: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	if (! S_ISDIR(my_inode.mode)){
		ERR(fprintf(stderr, "ERR: search_by_name: not a directory\n"));
		ERR(fprintf(stderr, "  my_inode.mode:           %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  S_ISDIR(my_inode.mode):  %d\n", S_ISDIR(my_inode.mode)));
		return NOT_DIR;
	}
	
	int cur_block = 0;
	int reached_end = FALSE;
	int num_entries, i;
	dirblock d;
	for (cur_block = 0; reached_end == FALSE; cur_block++){
		if (read_dir_page(inum, &d, cur_block, &num_entries, &reached_end) == SUCCESS){
			for (i = 0; i < num_entries; i++){
				if (strncmp(name, d.dir_ents[i].name, FILEBUF_SIZE) == 0){
					*index = cur_block * DIRENTS_PER_BLOCK + i;
					*target = d.dir_ents[i].inode_num;
					return SUCCESS;
				}
			}
		}
		else{
			ERR(fprintf(stderr, "ERR: search_by_name: read_dir failed\n"));
			return NOT_DIR;
		}
	}
	
	return NOT_IN_DIR;
}

/* Removes the index-th item from a directory
 *
 * Does this by moving the last item to its location and shrinking the directory
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BAD_INDEX          - index isn't a valid entry in this directory
 *   NOT_DIR            - S_IFDIR bit is not set
 *   SUCCESS            - upon success, returns the number of bytes written (sizeof(dir_ent))
 */
int remove_dirent(int inum, int index){
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: remove_dirent: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	if (! S_ISDIR(my_inode.mode)){
		ERR(fprintf(stderr, "ERR: remove_dirent: not a directory\n"));
		ERR(fprintf(stderr, "  my_inode.mode:           %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  S_ISDIR(my_inode.mode):  %d\n", S_ISDIR(my_inode.mode)));
		return NOT_DIR;
	}
	if (my_inode.size == 0 || index < 0){
		ERR(fprintf(stderr, "ERR: remove_dirent: invalid index\n"));
		ERR(fprintf(stderr, "  my_inode.size: %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  index:         %d\n", index));
		return BAD_INDEX;
	}
	
	/* Location of entry to be removed */
	int block_num = index / DIRENTS_PER_BLOCK;
	int block_offset = index % DIRENTS_PER_BLOCK;
	
	/* Location of the last entry */
	int num_blocks = my_inode.size / BLOCK_SIZE;
	int remainder = my_inode.size % BLOCK_SIZE;
	if (remainder % sizeof(dir_ent) != 0){
		ERR(fprintf(stderr, "ERR: read_dir: size not a multiple of dir_ent\n"));
		ERR(fprintf(stderr, "  sizeof(dir_ent): %d\n", sizeof(dir_ent)));
		ERR(fprintf(stderr, "  remainder:       %d\n", remainder));
		return MALFORMED_DIRECTORY;
	}
	int remainder_index = remainder / sizeof(dir_ent);
	
	/* Check that index specified is in the file */
	if ((block_num * BLOCK_SIZE) + (block_offset * sizeof(dir_ent)) > (num_blocks * BLOCK_SIZE) + (remainder - sizeof(dir_ent))){
		ERR(fprintf(stderr, "ERR: remove_dirent: invalid index\n"));
		ERR(fprintf(stderr, "  my_inode.size: %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  index:         %d\n", index));
		return BAD_INDEX;
	}
	
	/* Read the last item and write it at index */
	dir_ent last;
	read_i(inum, &last, (num_blocks * BLOCK_SIZE) + (remainder - sizeof(dir_ent)), sizeof(dir_ent));
	ret = write_i(inum, &last, (block_num * BLOCK_SIZE) + (block_offset * sizeof(dir_ent)), sizeof(dir_ent));
	if (ret != sizeof(dir_ent)){
		ERR(fprintf(stderr, "ERR: remove_dirent: write_i failed\n"));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	
	/* Update the size of the directory */
	int new_size = my_inode.size - sizeof(dir_ent);
	if (new_size % BLOCK_SIZE == 0 && new_size != 0){
		new_size = new_size - DIRENTS_REMAINDER;
	}
	trunc_fs(inum, new_size);
	
	return SUCCESS;
}

/* Adds an entry to a directory
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - d is null
 *   NOT_DIR            - S_IFDIR bit is not set
 *   DATA_FULL          - if a block cannot be written because the FS filled up
 *   INT                - upon success, returns the number of bytes written (sizeof(dir_ent))
 */
int add_dirent(int inum, dir_ent* d){
	if (d == NULL){
		ERR(fprintf(stderr, "ERR: add_dirent: d is null\n"));
		ERR(fprintf(stderr, "  d: %p\n", d));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: add_dirent: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	if (! S_ISDIR(my_inode.mode)){
		ERR(fprintf(stderr, "ERR: add_dirent: not a directory\n"));
		ERR(fprintf(stderr, "  my_inode.mode:           %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  S_ISDIR(my_inode.mode):  %d\n", S_ISDIR(my_inode.mode)));
		return NOT_DIR;
	}
	
	off_t write_offset = my_inode.size;
	if ((my_inode.size + sizeof(dir_ent)) / BLOCK_SIZE > my_inode.size / BLOCK_SIZE){
		write_offset += DIRENTS_REMAINDER;
	}

	int remainder = my_inode.size % BLOCK_SIZE;
	if (remainder % sizeof(dir_ent) != 0){
		ERR(fprintf(stderr, "ERR: read_dir: size not a multiple of dir_ent\n"));
		ERR(fprintf(stderr, "  sizeof(dir_ent): %d\n", sizeof(dir_ent)));
		ERR(fprintf(stderr, "  remainder:       %d\n", remainder));
		return MALFORMED_DIRECTORY;
	}
	
	return write_i(inum, d, write_offset, sizeof(dir_ent));
}

/* Deletes an inode and the data associated with it
 *
 * Returns (normally only SUCCESS):
 *   DISC_UNINITIALIZED  - no disk
 *   BAD_INODE           - inode supplied was bad
 *   SUCCESS             - file size changed
 */
int del(int inum){
	/* Delete the data */
	int ret = trunc_fs(inum, 0);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: delete: trunc_fs failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	
	/* Delete the inode */
	return inode_free(inum);
}

/* Changes a file's size to exactly offset bytes
 *
 * Appends 0s to a file if offset > current file size
 *
 * Returns (normally only SUCCESS):
 *   DISC_UNINITIALIZED  - no disk
 *   BAD_INODE           - inode supplied was bad
 *   SUCCESS             - file size changed
 */
int trunc_fs(int inum, off_t offset){
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_dir: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	
	/* Does the file need to be extended */
	if (offset > my_inode.size){
		my_inode.size = offset;
		inode_write(inum, &my_inode);
		return SUCCESS;
	}

	/* Shorten the file */
	off_t buf_size = BLOCK_SIZE * 10;
	off_t len_written = 0;
	off_t max_written = my_inode.size - offset;

	uint8_t buf[buf_size];
	memset(buf, 0, buf_size);
	
	while(len_written < max_written){
		write_i(inum, buf, offset + len_written, buf_size);
		len_written += buf_size;
	}

	my_inode.size = offset;
	inode_write(inum, &my_inode);
	
	return SUCCESS;
}

/* Reads a block of directory entries from a directory into d
 *
 * Returns (normally only SUCCESS):
 *   DISC_UNINITIALIZED  - no disk
 *   BUF_NULL            - one of the given parameters is null
 *   INVALID_PAGE        - page is negative or beyond the end of the file
 *   NOT_DIR             - S_IFDIR bit is not set
 *   MALFORMED_DIRECTORY - the directory is of odd size
 *   SUCCESS             - directory was read successfully
 */
int read_dir_page(int inum, dirblock* d, int page, int* entries, int* last){
	if (d == NULL || entries == NULL || last == NULL){
		ERR(fprintf(stderr, "ERR: read_dir_page: something is null\n"));
		ERR(fprintf(stderr, "  d:       %p\n", d));
		ERR(fprintf(stderr, "  entries: %p\n", entries));
		ERR(fprintf(stderr, "  last:    %p\n", last));
		return BUF_NULL;
	}

	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_dir_page: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	
	if (! S_ISDIR(my_inode.mode)){
		ERR(fprintf(stderr, "ERR: read_dir_page: not a directory\n"));
		ERR(fprintf(stderr, "  my_inode.mode:           %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  S_ISDIR(my_inode.mode):  %d\n", S_ISDIR(my_inode.mode)));
		return NOT_DIR;
	}
	if (my_inode.size == 0){
		ERR(fprintf(stderr, "ERR: read_dir_page: size is 0\n"));
		return MALFORMED_DIRECTORY;
	}
	
	int num_blocks = my_inode.size / BLOCK_SIZE;
	int remainder = my_inode.size % BLOCK_SIZE;
	if (remainder % sizeof(dir_ent) != 0){
		ERR(fprintf(stderr, "ERR: read_dir_page: size not a multiple of dir_ent\n"));
		ERR(fprintf(stderr, "  sizeof(dir_ent): %d\n", sizeof(dir_ent)));
		ERR(fprintf(stderr, "  remainder:       %d\n", remainder));
		return MALFORMED_DIRECTORY;
	}
	if (page > num_blocks || page < 0){
		ERR(fprintf(stderr, "ERR: read_dir_page: page invalid\n"));
		ERR(fprintf(stderr, "  num_blocks: %d\n", num_blocks));
		ERR(fprintf(stderr, "  page:       %d\n", page));
		return INVALID_PAGE;
	}
	
	
	/* Figure out if this is the last block or not */
	if (page == num_blocks){
		*entries = remainder / sizeof(dir_ent);
		*last = TRUE;
	}
	else{
		*entries = DIRENTS_PER_BLOCK;
		*last = FALSE;
	}
	
	/* Read a block into the dirblock object */
	ret = read_i(inum, d, page * BLOCK_SIZE, *entries * sizeof(dir_ent));
	if (ret != *entries * sizeof(dir_ent)){
		ERR(fprintf(stderr, "ERR: read_dir_page: read_i failed\n"));
		ERR(fprintf(stderr, "  inum:   %d\n", inum));
		ERR(fprintf(stderr, "  d:    %p\n", d));
		ERR(fprintf(stderr, "  offset: %d\n", page * BLOCK_SIZE));
		ERR(fprintf(stderr, "  ret:    %d\n", ret));
		return ret;
	}
	
	return SUCCESS;
}

/* Reads all directory entries from a directory into buf
 *
 * Returns (normally only SUCCESS):
 *   DISC_UNINITIALIZED  - no disk
 *   BUF_NULL            - read buffer is null
 *   NOT_DIR             - S_IFDIR bit is not set
 *   MALFORMED_DIRECTORY - the directory is of odd size
 *   SUCCESS             - directory was read successfully
 */
int read_dir_whole(int inum, dir_ent* buf){
	if (buf == NULL){
		ERR(fprintf(stderr, "ERR: read_dir_whole: buf is null\n"));
		ERR(fprintf(stderr, "  buf: %p\n", buf));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_dir_whole: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		ERR(fprintf(stderr, "  ret:  %d\n", ret));
		return ret;
	}
	
	if (! S_ISDIR(my_inode.mode)){
		ERR(fprintf(stderr, "ERR: read_dir_whole: not a directory\n"));
		ERR(fprintf(stderr, "  my_inode.mode:           %d\n", my_inode.mode));
		ERR(fprintf(stderr, "  S_ISDIR(my_inode.mode):  %d\n", S_ISDIR(my_inode.mode)));
		return NOT_DIR;
	}
	if (my_inode.size == 0){
		ERR(fprintf(stderr, "ERR: read_dir_whole: size is 0\n"));
		return MALFORMED_DIRECTORY;
	}
	
	int num_blocks = my_inode.size / BLOCK_SIZE;
	int remainder = my_inode.size % BLOCK_SIZE;
	if (remainder % sizeof(dir_ent) != 0){
		ERR(fprintf(stderr, "ERR: read_dir_whole: size not a multiple of dir_ent\n"));
		ERR(fprintf(stderr, "  sizeof(dir_ent): %d\n", sizeof(dir_ent)));
		ERR(fprintf(stderr, "  remainder:       %d\n", remainder));
		return MALFORMED_DIRECTORY;
	}

	/* Read full blocks of directory entries */
	int i;
	uintptr_t write_offset = 0;
	for (i = 0; i < num_blocks; i++){
		ret = read_i(inum, (void*)((uintptr_t)buf + write_offset), i * BLOCK_SIZE, DIRENTS_PER_BLOCK * sizeof(dir_ent));
		if (ret != DIRENTS_PER_BLOCK * sizeof(dir_ent)){
			ERR(fprintf(stderr, "ERR: read_dir_whole: read_i failed\n"));
			ERR(fprintf(stderr, "  inum:   %d\n", inum));
			ERR(fprintf(stderr, "  buf:    %d\n", (void*)((uintptr_t)buf + write_offset)));
			ERR(fprintf(stderr, "  offset: %d\n", i * BLOCK_SIZE));
			ERR(fprintf(stderr, "  size:   %d\n", DIRENTS_PER_BLOCK * sizeof(dir_ent)));
			ERR(fprintf(stderr, "  ret:    %d\n", ret));
			return ret;
		}
		write_offset += DIRENTS_PER_BLOCK * sizeof(dir_ent);
	}
	
	/* Read partial directory entries */
	ret = read_i(inum, (void*)((uintptr_t)buf + write_offset), i * BLOCK_SIZE, remainder);
	if (ret != remainder){
		ERR(fprintf(stderr, "ERR: read_dir_whole: read_i failed\n"));
		ERR(fprintf(stderr, "  inum:   %d\n", inum));
		ERR(fprintf(stderr, "  buf:    %d\n", (void*)((uintptr_t)buf + write_offset)));
		ERR(fprintf(stderr, "  offset: %d\n", i * BLOCK_SIZE));
		ERR(fprintf(stderr, "  size:   %d\n", remainder));
		ERR(fprintf(stderr, "  ret:    %d\n", ret));
		return ret;
	}
	
	return SUCCESS;
}

/* Reads size data from inum at offset offset into buf
 *
 * Returns (normally only INT):
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - read buffer is null
 *   INVALID_BLOCK      - a data block found was invalid
 *   INT                - upon success, returns number of bytes read
 */
int read_i(int inum, void* buf, off_t offset, size_t size){
	if (buf == NULL){
		ERR(fprintf(stderr, "ERR: read_i: buf is null\n"));
		ERR(fprintf(stderr, "  buf: %p\n", buf));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: read_i: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		return ret;
	}

	/* If the read position is beyond the end of the file */
	if (offset >= my_inode.size){
		return 0;
	}
	
	/* Consider the length we will read */
	int truncated_size = MIN(my_inode.size - offset, size);
	if (truncated_size == 0){
		return 0;
	}
	
	/* Calculate reading start and end */
	int start_block = offset / BLOCK_SIZE;
	int start_offset = offset % BLOCK_SIZE;

	int end_block = (offset + truncated_size - 1) / BLOCK_SIZE; //not sure this is correct
	int end_size = ((offset + truncated_size - 1) % BLOCK_SIZE) + 1; //1-4096
	
	DEBUG(DB_READI, printf("DEBUG: read_i: calculated offsets\n"));
	DEBUG(DB_READI, printf("  offset:              %d\n", offset));
	DEBUG(DB_READI, printf("  size:                %d\n", size));
	DEBUG(DB_READI, printf("  start_block:         %d\n", start_block));
	DEBUG(DB_READI, printf("  start_offset:        %d\n", start_offset));
	DEBUG(DB_READI, printf("  end_block:           %d\n", end_block));
	DEBUG(DB_READI, printf("  end_size:            %d\n", end_size));

	int block_addr;
	uint8_t block_buf[BLOCK_SIZE];
	
	int read_start = start_offset;
	uintptr_t read_size = BLOCK_SIZE;
	uintptr_t bytes_read = 0;
	
	/* Read blocks */
	int i;
	for (i = start_block; i <= end_block; i++){
		if (i == end_block){
			read_size = end_size;
		}
		
		block_addr = get_nth_datablock(&my_inode, i, FALSE, NULL);
		DEBUG(DB_READI, printf("  block_addr (%06d): %d\n", i, block_addr));
		
		ret = data_read(block_addr, block_buf);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: read_i: data_read failed\n"));
			ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
			return ret;
		}
		
		memcpy((void*)((uintptr_t)buf + bytes_read), (void*)((uintptr_t)block_buf + read_start), read_size - read_start);
		
		bytes_read += read_size - read_start;
		
		read_start = 0;
		read_size = BLOCK_SIZE;
	}
	
	return bytes_read;
}

/* Writes size bytes at offset offset from buf into the file specified by inum
 *
 * Updates inode's size field and clears its SUID and SGID bits
 *
 * Returns (normally only DATA_FULL or INT):
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - buf is null
 *   INVALID_BLOCK      - a data block found was invalid
 *   DATA_FULL          - if a block cannot be written because the FS filled up
 *   INT                - upon success, returns the number of bytes written
 */
int write_i(int inum, void* buf, off_t offset, size_t size){
	if (buf == NULL){
		ERR(fprintf(stderr, "ERR: write_i: buf is null\n"));
		ERR(fprintf(stderr, "  buf: %p\n", buf));
		return BUF_NULL;
	}
	
	int ret;
	inode my_inode;
	ret = inode_read(inum, &my_inode);
	if (ret != SUCCESS){
		ERR(fprintf(stderr, "ERR: write_i: inode_read failed\n"));
		ERR(fprintf(stderr, "  inum: %d\n", inum));
		return ret;
	}

	int original_size = my_inode.size;
	
	/* Calculate writing start and end */
	int start_block = offset / BLOCK_SIZE;
	int start_offset = offset % BLOCK_SIZE;

	int end_block = (offset + size - 1) / BLOCK_SIZE; //not sure this is correct
	int end_size = ((offset + size - 1) % BLOCK_SIZE) + 1; //1-4096
	
	DEBUG(DB_WRITEI, printf("DEBUG: write_i: calculated offsets\n"));
	DEBUG(DB_WRITEI, printf("  offset:              %d\n", offset));
	DEBUG(DB_WRITEI, printf("  size:                %d\n", size));
	DEBUG(DB_WRITEI, printf("  original_size:       %d\n", original_size));
	DEBUG(DB_WRITEI, printf("  start_block:         %d\n", start_block));
	DEBUG(DB_WRITEI, printf("  start_offset:        %d\n", start_offset));
	DEBUG(DB_WRITEI, printf("  end_block:           %d\n", end_block));
	DEBUG(DB_WRITEI, printf("  end_size:            %d\n", end_size));

	uint8_t zero_block[BLOCK_SIZE];
	memset(zero_block, 0, BLOCK_SIZE);
	
	uint8_t block_buf[BLOCK_SIZE];
	
	int write_start = start_offset;
	uintptr_t write_size = BLOCK_SIZE;
	uintptr_t bytes_written = 0;
	int created, all_zeros, block_addr;

	/* Write intermediate full blocks */
	int i;
	for (i = start_block; i <= end_block; i++){
		//if we're writing a block of 0s, just delete the block and move on
		//get the data block
		  //if this fails BUT we're writing all 0s -> delete it and continue
		  //if this fails and we're not writing all 0s -> retun FS full
		//add our data to it (0s if created, read it if not created)
		//write it back, if all 0s -> delete it
		if (i == end_block){
			write_size = end_size;
		}
		
		/* Check to see if we're writing all 0s */
		if (memcmp((void*)((uintptr_t)buf + write_start), zero_block, write_size - write_start) == 0){
			all_zeros = TRUE;
		}
		else{
			all_zeros = FALSE;
		}
		
		/* If we're writing a block of 0s, just delete it instead */
		if (all_zeros && write_size == BLOCK_SIZE && write_start == 0){
			rm_nth_datablock(&my_inode, i);
			block_addr = 0;
		}
		else{
			/* If the filesystem is full and we aren't writing all zeros */
			created = FALSE;
			block_addr = get_nth_datablock(&my_inode, i, TRUE, &created);
			if (block_addr == DATA_FULL && !all_zeros){
				rm_nth_datablock(&my_inode, i);
				ERR(fprintf(stderr, "ERR: write_i: filesystem full\n"));
				return DATA_FULL;
			}
			/* If the filesystem is full but we are writing all zeros */
			else if (block_addr == DATA_FULL){
				rm_nth_datablock(&my_inode, i);
				block_addr = 0;
			}
			/* If the filesystem isn't full */
			else{
				/* If we're writing a whole block, just write it directly from buf */
				if (write_size == BLOCK_SIZE && write_start == 0){
					ret = data_write(block_addr, (void*)((uintptr_t)buf + bytes_written));
					if (ret != SUCCESS){
						ERR(fprintf(stderr, "ERR: write_i: data_write failed\n"));
						ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
						ERR(fprintf(stderr, "  block_buf:  %p\n", block_buf));
						return ret;
					}
				}
				/* If we aren't writing a whole block, read it first, add our data, then write it back */
				else{
					/* Fresh block -> starts out all 0s */
					if (created){
						memset(block_buf, 0, BLOCK_SIZE);
					}
					else{
						ret = data_read(block_addr, &block_buf);
						if (ret != SUCCESS){
							ERR(fprintf(stderr, "ERR: write_i: data_read failed\n"));
							ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
							return ret;
						}
					}
					
					/* If the resultant block is all 0s, just delete it instead */
					memcpy((void*)(block_buf + (uintptr_t)write_start), (void*)((uintptr_t)buf + bytes_written), write_size - write_start);
					if (memcmp(block_buf, zero_block, BLOCK_SIZE) == 0){
						rm_nth_datablock(&my_inode, i);
						block_addr = 0;
					}
					else{
						ret = data_write(block_addr, &block_buf);
						if (ret != SUCCESS){
							ERR(fprintf(stderr, "ERR: write_i: data_write failed\n"));
							ERR(fprintf(stderr, "  block_addr: %d\n", block_addr));
							ERR(fprintf(stderr, "  block_buf:  %p\n", block_buf));
							return ret;
						}
					}
				}
			}
		}
		
		DEBUG(DB_WRITEI, printf("  block_addr (%06d): %d\n", i, block_addr));
		
		bytes_written += write_size - write_start;
		
		write_start = 0;
		write_size = BLOCK_SIZE;
		
		/* Update and write inode */
		my_inode.size = MAX(original_size, offset + bytes_written);
		my_inode.mode &= (0xffff ^ (S_ISUID | S_ISGID));
		inode_write(inum, &my_inode);
	}

	return bytes_written;
}

/* Frees the blocks associated with the nth block of a file. Does not return
 * an error if the block is already deleted
 *
 * This operation is equivilant (from a file contents perspective) of writing all
 * 0s to a block, except that it doesn't take up room on disk
 *
 * Note that n is zero-indexed. n = 0 will return the first block of the file
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - inode is null
 *   INVALID_BLOCK      - an invalid block number was found during the search
 *   SUCCESS            - nth block of file is no longer allocated
 */
int rm_nth_datablock(inode* inod, off_t n){
	if (inod == NULL){
		ERR(fprintf(stderr, "ERR: rm_nth_datablock: inode is null\n"));
		ERR(fprintf(stderr, "  inod: %p\n", inod));
		return BUF_NULL;
	}
	
	int num_direct = NUM_DIRECT; //10
	int single_indirect = num_direct + ADDRESSES_PER_BLOCK; //10 + 1024
	int double_indirect = single_indirect + ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK; //10 + 1024 + 1048576
	int triple_indirect = double_indirect + ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK;
	
	address_block b[3];
	int offset, next_addr, next_index[3], ret, block_addrs[3];
	int* from_pointer;
	int check_start = 0;
	
	uint8_t empty_block[BLOCK_SIZE];
	memset(empty_block, 0, BLOCK_SIZE);
	
	DEBUG(DB_RMNTH, printf("DEBUG: rm_nth_datablock: about to begin\n"));
	DEBUG(DB_RMNTH, printf("  n:                %d\n", n));
	DEBUG(DB_RMNTH, printf("  num_direct:       %d\n", num_direct));
	DEBUG(DB_RMNTH, printf("  single_indirect:  %d\n", single_indirect));
	DEBUG(DB_RMNTH, printf("  double_indirect:  %d\n", double_indirect));
	DEBUG(DB_RMNTH, printf("  triple_indirect:  %d\n", triple_indirect));
	DEBUG(DB_RMNTH, printf("  inod:             %p\n", inod));
	
	/* Figure out what type of block our target is, update i and next_addr */
	offset = n;
	int i;
	if (offset < num_direct){
		i = -1;
		next_addr = inod->direct_blocks[offset];
		from_pointer = &inod->direct_blocks[offset];
	}
	else if (offset < single_indirect){
		offset -= num_direct;
		i = 0;
		next_addr = inod->indirect;
		from_pointer = &inod->indirect;
	}
	else if (offset < double_indirect){
		offset -= single_indirect;
		i = 1;
		next_addr = inod->double_indirect;
		from_pointer = &inod->double_indirect;
	}
	else if (offset < triple_indirect){
		offset -= double_indirect;
		i = 2;
		next_addr = inod->triple_indirect;
		from_pointer = &inod->triple_indirect;
	}
	else{
		ERR(fprintf(stderr, "ERR: rm_nth_datablock: block num was too big\n"));
		ERR(fprintf(stderr, "  triple_indirect: %d\n", triple_indirect));
		ERR(fprintf(stderr, "  n:               %d\n", n));
		return INVALID_BLOCK;
	}
	
	DEBUG(DB_RMNTH, printf("  next_addr:        %d\n", next_addr));
	
	/* Continue down indirect blocks */
	int j;
	for (j = i; j >= 0; j--){
		/* Don't bother checking down 0 blocks */
		if (next_addr == 0){
			DEBUG(DB_RMNTH, printf("DEBUG: rm_nth_datablock: block is zero\n"));
			DEBUG(DB_RMNTH, printf("  j: %d\n", j));
			check_start = j + 1;
			break;
		}
		
		block_addrs[j] = next_addr;
		ret = data_read(next_addr, &b[j]);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: get_nth_datablock: data_read failed\n"));
			ERR(fprintf(stderr, "  next_addr: %d\n", next_addr));
			ERR(fprintf(stderr, "  ret:       %d\n", ret));
			return ret;
		}
		
		next_index[j] = offset / intPow(ADDRESSES_PER_BLOCK, j);
		offset = offset % intPow(ADDRESSES_PER_BLOCK, j);
		
		DEBUG(DB_RMNTH, printf("DEBUG: rm_nth_datablock: doing an iteration\n"));
		DEBUG(DB_RMNTH, printf("  j:              %d\n", j));
		DEBUG(DB_RMNTH, printf("  next_index[j]:  %d\n", next_index[j]));
		DEBUG(DB_RMNTH, printf("  offset:         %d\n", offset));
		
		next_addr = b[j].address[next_index[j]];
		
		DEBUG(DB_RMNTH, printf("  next_addr:      %d\n", next_addr));
	}
	
	/* Delete the blocks that need deleting */
	for (j = check_start; j <= i; j++){
		next_addr = b[j].address[next_index[j]];
		if (next_addr != 0){
			ret = data_free(next_addr);
			if (ret != SUCCESS){
				ERR(fprintf(stderr, "ERR: rm_nth_datablock: data_free failed\n"));
				ERR(fprintf(stderr, "  next_addr: %d\n", next_addr));
				ERR(fprintf(stderr, "  ret:       %d\n", ret));
				return ret;
			}
			
			DEBUG(DB_RMNTH, printf("DEBUG: rm_nth_datablock: freeing a block\n"));
			DEBUG(DB_RMNTH, printf("  j:              %d\n", j));
			DEBUG(DB_RMNTH, printf("  next_addr:      %d\n", next_addr));
			DEBUG(DB_RMNTH, printf("  next_index[j]:  %d\n", next_index[j]));
			DEBUG(DB_RMNTH, printf("  block_addrs[j]: %d\n", block_addrs[j]));
			
			b[j].address[next_index[j]] = 0;
			data_write(block_addrs[j], &b[j]);
		}
		
		/* If all of b[j] is now 0s, we can delete it as well */
		if (memcmp(&b[j], empty_block, BLOCK_SIZE) != 0){
			return SUCCESS;
		}
		DEBUG(DB_RMNTH, printf("DEBUG: rm_nth_datablock: recursively freeing\n"));
	}

	/* Clear the pointer in inode, if needed */
	if (*from_pointer != 0){
		DEBUG(DB_RMNTH, printf("DEBUG: rm_nth_datablock: clearing inode\n"));
		DEBUG(DB_RMNTH, printf("  *from_pointer:  %d\n", *from_pointer));

		ret = data_free(*from_pointer);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: rm_nth_datablock: data_free failed\n"));
			ERR(fprintf(stderr, "  *from_pointer: %d\n", *from_pointer));
			ERR(fprintf(stderr, "  ret:           %d\n", ret));
			return ret;
		}
		
		*from_pointer = 0;
	}

	return SUCCESS;
 }

/* Returns the data block number associated with the nth block of a file
 *
 * If create is true, get_nth_datablock will attempt to create the datablock, if
 * it doesn't exist. Doing this may update inod and data blocks in the filesystem.
 * If a data block is created, created will be set to TRUE, otherwise it is unchanged
 *
 * Note that n is zero-indexed. n = 0 will return the first block of the file
 *
 * Returns:
 *   DISC_UNINITIALIZED - no disk
 *   BUF_NULL           - inode is null
 *   DATA_FULL          - if create is true and the block (or an intermediate block) cannot be created
 *   INVALID_BLOCK      - an invalid block number was found during the search
 *   INT                - upon success, returns number for data block
 */
int get_nth_datablock(inode* inod, off_t n, int create, int* created){
	if (inod == NULL){
		ERR(fprintf(stderr, "ERR: get_nth_datablock: inode is null\n"));
		ERR(fprintf(stderr, "  inod: %p\n", inod));
		return BUF_NULL;
	}
	
	int num_direct = NUM_DIRECT; //10
	int single_indirect = num_direct + ADDRESSES_PER_BLOCK; //10 + 1024
	int double_indirect = single_indirect + ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK; //10 + 1024 + 1048576
	int triple_indirect = double_indirect + ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK * ADDRESSES_PER_BLOCK;
	
	address_block b;
	int offset, next_addr, next_index, ret, new_block;
	int* from_pointer;
	
	uint8_t empty_block[BLOCK_SIZE];
	memset(empty_block, 0, BLOCK_SIZE);
	
	DEBUG(DB_GETNTH, printf("DEBUG: get_nth_datablock: about to begin\n"));
	DEBUG(DB_GETNTH, printf("  n:                %d\n", n));
	DEBUG(DB_GETNTH, printf("  num_direct:       %d\n", num_direct));
	DEBUG(DB_GETNTH, printf("  single_indirect:  %d\n", single_indirect));
	DEBUG(DB_GETNTH, printf("  double_indirect:  %d\n", double_indirect));
	DEBUG(DB_GETNTH, printf("  triple_indirect:  %d\n", triple_indirect));
	DEBUG(DB_GETNTH, printf("  inod:             %p\n", inod));
	
	/* Figure out what type of block our target is, update i and next_addr */
	offset = n;
	int i;
	if (offset < num_direct){
		i = -1;
		next_addr = inod->direct_blocks[offset];
		from_pointer = &inod->direct_blocks[offset];
	}
	else if (offset < single_indirect){
		offset -= num_direct;
		i = 0;
		next_addr = inod->indirect;
		from_pointer = &inod->indirect;
	}
	else if (offset < double_indirect){
		offset -= single_indirect;
		i = 1;
		next_addr = inod->double_indirect;
		from_pointer = &inod->double_indirect;
	}
	else if (offset < triple_indirect){
		offset -= double_indirect;
		i = 2;
		next_addr = inod->triple_indirect;
		from_pointer = &inod->triple_indirect;
	}
	else{
		ERR(fprintf(stderr, "ERR: get_nth_datablock: block num was too big\n"));
		ERR(fprintf(stderr, "  triple_indirect: %d\n", triple_indirect));
		ERR(fprintf(stderr, "  n:               %d\n", n));
		return INVALID_BLOCK;
	}
	
	DEBUG(DB_GETNTH, printf("  next_addr:        %d\n", next_addr));

	/* Special case when creating a first-layer block, since we have to update the inode */
	if (next_addr == 0 && create){
		/* Create a new block of all zeros */
		ret = data_allocate(&empty_block, &new_block);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: get_nth_datablock: filesystem full\n"));
			ERR(fprintf(stderr, "  ret: %d\n", ret));
			return ret;
		}

		DEBUG(DB_GETNTH, printf("  DEBUG: get_nth_datablock: created a new block\n"));
		DEBUG(DB_GETNTH, printf("    new_block: %d\n", new_block));
		
		/* Update values to point to new block */
		*from_pointer = new_block;
		next_addr = new_block;
		
		/* Do not update the inode itself, that's the caller's job */
		*created = TRUE;
	}
	
	/* Continue down indirect blocks */
	for (; i >= 0; i--){
		if (next_addr == 0 && !create) return 0;
		ret = data_read(next_addr, &b);
		if (ret != SUCCESS){
			ERR(fprintf(stderr, "ERR: get_nth_datablock: data_read failed\n"));
			ERR(fprintf(stderr, "  next_addr:            %d\n", next_addr));
			ERR(fprintf(stderr, "  ret:                  %d\n", ret));
			return ret;
		}
		
		next_index = offset / intPow(ADDRESSES_PER_BLOCK, i);
		offset = offset % intPow(ADDRESSES_PER_BLOCK, i);
		
		DEBUG(DB_GETNTH, printf("DEBUG: get_nth_datablock: doing an iteration\n"));
		DEBUG(DB_GETNTH, printf("  i:          %d\n", i));
		DEBUG(DB_GETNTH, printf("  next_index: %d\n", next_index));
		DEBUG(DB_GETNTH, printf("  offset:     %d\n", offset));
		
		/* If the next block needs to be created */
		if (b.address[next_index] == 0 && create){
			/* Create a new block of all zeros */
			ret = data_allocate(&empty_block, &new_block);
			if (ret != SUCCESS){
				ERR(fprintf(stderr, "ERR: get_nth_datablock: filesystem full\n"));
				ERR(fprintf(stderr, "  ret: %d\n", ret));
				return ret;
			}

			DEBUG(DB_GETNTH, printf("  DEBUG: get_nth_datablock: created a new block\n"));
			DEBUG(DB_GETNTH, printf("    new_block: %d\n", new_block));
			
			/* Update b and write it back */
			b.address[next_index] = new_block;
			ret = data_write(next_addr, &b);
			if (ret != SUCCESS){
				ERR(fprintf(stderr, "ERR: get_nth_datablock: tried to write an invalid block\n"));
				ERR(fprintf(stderr, "  next_addr: %d\n", next_addr));
				ERR(fprintf(stderr, "  ret:       %d\n", ret));
				return ret;
			}
			*created = TRUE;
		}
		
		next_addr = b.address[next_index];
		
		DEBUG(DB_GETNTH, printf("  next_addr:  %d\n", next_addr));
	}
	
	return next_addr;
 }
 
/* Simple helper function to calculate pow for positive ints */
int intPow(int x, int y){
	int sum = 1;
	int i;
	for (i = 0; i < y; i++){
		sum *= x;
	}

	return sum;
}