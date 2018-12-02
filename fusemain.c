#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include "globals.h"
#include "layer0.h"
#include "layer1.h"
#include "layer2.h"

static void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg){
	struct fuse_context* context = fuse_get_context();
	mkfs(400, context->uid, context->gid);
	return NULL;
}

static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
	struct fuse_context* context = fuse_get_context();
	
	int parent_inum, target_inum, index, ret, read, write, exec;
	ret = namei(path, context->uid, context->gid, &parent_inum, &target_inum, &index);
	if (ret != SUCCESS){
		return ret;
	}
	
	if (target_inum == INVALID_INODE){
		return -ENOENT;
	}
	
	/* Check for read permissions */
	check_permissions(parent_inum, context->uid, context->gid, &read, &write, &exec);
	if (!read){
		return -EACCES;
	}
	
	struct stat s = get_stat(target_inum);
	memcpy(stbuf, &s, sizeof(struct stat));
	
	return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags){
	struct fuse_context* context = fuse_get_context();
	
	int parent_inum, target_inum, index, ret, read, write, exec;
	ret = namei(path, context->uid, context->gid, &parent_inum, &target_inum, &index);
	if (ret != SUCCESS){
		return ret;
	}
	
	if (target_inum == INVALID_INODE){
		return -ENOENT;
	}
	
	/* Check that it is a directory */
	inode my_inode;
	ret = inode_read(target_inum, &my_inode);
	if ((my_inode.mode & S_IFDIR) == 0){
		return -ENOTDIR;
	}
	
	int last = FALSE;
	int entries = 0;
	struct stat s;
	dirblock d;
	int page = 0;
	while (last == FALSE){
		ret = read_dir_page(target_inum, &d, page, &entries, &last);
		for (i = 0; i < entries; i++){
			s = get_stat(d.dir_ents[i].inode_num);
			
			if (filler(buf, d.dir_ents[i].name, &s, 0, 0)){
				return 0;
			}
		}
		
		page++;
	}
	
	return 0;
}

/************************************************************************************ CREATION FUNCTIONS ************************************************************************************/

static int fs_mknod(const char *path, mode_t mode, dev_t rdev){
	struct fuse_context* context = fuse_get_context();
	
	return mknod_fs(path, mode, context->uid, context->gid);
}

static int fs_mkdir(const char *path, mode_t mode){
	struct fuse_context* context = fuse_get_context();
	
	return mkdir_fs(path, mode, context->uid, context->gid);
}


/************************************************************************************ REMOVAL FUNCTIONS ************************************************************************************/


static int fs_unlink(const char *path){
	struct fuse_context* context = fuse_get_context();
	
	int parent_inum, target_inum, index, ret, read, write, exec;
	ret = namei(path, context->uid, context->gid, &parent_inum, &target_inum, &index);
	if (ret != SUCCESS){
		return ret;
	}

	if (target_inum == INVALID_INODE){
		return -ENOENT;
	}
	
	/* Check that it isn't a directory */
	inode my_inode;
	ret = inode_read(target_inum, &my_inode);
	if ((my_inode.mode & S_IFDIR) != 0){
		return -ENOTDIR;
	}
	
	/* Check for write permissions on parent directory */
	check_permissions(parent_inum, context->uid, context->gid, &read, &write, &exec);
	if (!write){
		return -EACCES;
	}

	remove_dirent(parent_inum, index);
	oft_attempt_delete(target_inum);
	
	return 0;
}

static int fs_rmdir(const char *path){
	struct fuse_context* context = fuse_get_context();
	
	int parent_inum, target_inum, index, ret, read, write, exec;
	ret = namei(path, context->uid, context->gid, &parent_inum, &target_inum, &index);
	if (ret != SUCCESS){
		return ret;
	}

	if (target_inum == INVALID_INODE){
		return -ENOENT;
	}
	
	/* Check that it is a directory */
	inode my_inode;
	ret = inode_read(target_inum, &my_inode);
	if ((my_inode.mode & S_IFDIR) == 0){
		return -ENOTDIR;
	}
	
	/* Check for write permissions on parent directory */
	check_permissions(parent_inum, context->uid, context->gid, &read, &write, &exec);
	if (!write){
		return -EACCES;
	}

	remove_dirent(parent_inum, index);
	del(target_inum);
	
	return 0;
}

static int fs_truncate(const char *path, off_t size, struct fuse_file_info *fi){
	struct fuse_context* context = fuse_get_context();
	
	int parent_inum, target_inum, index, ret, read, write, exec;
	ret = namei(path, context->uid, context->gid, &parent_inum, &target_inum, &index);
	if (ret != SUCCESS){
		return ret;
	}

	if (target_inum == INVALID_INODE){
		return -ENOENT;
	}
	
	/* Check that it isn't a directory */
	inode my_inode;
	ret = inode_read(target_inum, &my_inode);
	if ((my_inode.mode & S_IFDIR) != 0){
		return -EISDIR;
	}
	
	/* Check for write permissions */
	check_permissions(target_inum, context->uid, context->gid, &read, &write, &exec);
	if (!write){
		return -EACCES;
	}

	truncate(target_inum, size);
	
	return 0;
}


/************************************************************************************ WRITING FUNCTIONS ************************************************************************************/


static int fs_open(const char *path, struct fuse_file_info *fi){
	struct fuse_context* context = fuse_get_context();
	
	int parent_inum, target_inum, index, ret, read, write, exec;
	ret = namei(path, context->uid, context->gid, &parent_inum, &target_inum, &index);
	if (ret != SUCCESS){
		return ret;
	}

	if (target_inum == INVALID_INODE){
		return -ENOENT;
	}

	/* Check for permissions */
	check_permissions(target_inum, context->uid, context->gid, &read, &write, &exec);
	int access_mode = fi->flags & O_ACCMODE;
	if (acces_mode == O_RDONLY){
		if (!read){
			return -EACCES;
		}
	}
	else if (acces_mode == O_WRONLY){
		if (!write){
			return -EACCES;
		}
	}
	else if (acces_mode == O_RDWR){
		if (!read || !write){
			return -EACCES;
		}
	}
	else{
		return -EINVAL;
	}
	
	/* Check that it isn't a directory */
	inode my_inode;
	ret = inode_read(target_inum, &my_inode);
	if (((my_inode.mode & S_IFDIR) != 0) && (access_mode == O_WRONLY || acces_mode == O_RDWR)){
		return -EISDIR;
	}

	fi->fh = oft_add(target_inum, fi->flags);
	
	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	if (fi == NULL){
		return -EBADF;
	}
	
	int fd = fi->fh;
	int flags, inode;
	if (oft_lookup(fd, &flags, &inode) == FALSE){
		return -EBADF;
	}

	int access_mode = fi->flags & O_ACCMODE;
	if (acces_mode != O_WRONLY && access_mode != O_RDONLY){
		return -EBADF;
	}
	
	return read_i(inode, buf, offset, size);
}

static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	if (fi == NULL){
		return -EBADF;
	}
	
	int fd = fi->fh;
	int flags, inode, ret;
	off_t actual_offset = offset;
	if (oft_lookup(fd, &flags, &inode) == FALSE){
		return -EBADF;
	}

	int access_mode = fi->flags & O_ACCMODE;
	if (acces_mode != O_WRONLY && access_mode != O_WRONLY){
		return -EBADF;
	}
	
	/* If the append flag is set, move offset to the end of the file */
	if ((flags & O_APPEND) != 0){
		inode my_inode;
		ret = inode_read(inum, &my_inode);
		if (ret != SUCCESS){
			return -EBADF;
		}
		actual_offset = my_inode.size;
	}
	
	return write_i(inode, buf, actual_offset, size);
}

static int fs_release(const char *path, struct fuse_file_info *fi){
	if (fi == NULL){
		return -EBADF;
	}

	oft_delete(fi->fh);
	
	return 0;
}


/************************************************************************************ FUSE FUNCTIONS ************************************************************************************/


static struct fuse_operations fs_oper = {
	.init       = fs_init,
	.getattr	= fs_getattr,
	.readdir	= fs_readdir,
	.mknod		= fs_mknod,
	.mkdir		= fs_mkdir,
	.unlink		= fs_unlink,
	.rmdir		= fs_rmdir,
	.truncate	= fs_truncate,
	.open		= fs_open,
	.create 	= fs_create,
	.read		= fs_read,
	.write		= fs_write,
	.release	= fs_release,
};

int main(int argc, char** argv){
	return fuse_main(argc, argv, &fs_oper, NULL);
}