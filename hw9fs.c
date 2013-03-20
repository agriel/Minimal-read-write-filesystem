#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>

#define HW9FS_MAGIC 0x38538538


int hw9fs_open(struct inode *inode, struct file* file) {
		printk(KERN_INFO "hw9fs_open called\n");	
		return 0;
}

/* this is the mapping function from inode block to disk block */
static int 
hw9fs_get_block(struct inode *inode, sector_t block,
							 struct buffer_head *bh_result, int create) {
	printk(KERN_INFO "hw9fs_getblock called\n");
	map_bh(bh_result,inode->i_sb,1);
	
	return 0;
}

/* called by VFS via generic_file_aio_read. Actual call is here
	 http://lxr.linux.no/linux+v2.6.38/mm/readahead.c#L127 */
static int
hw9fs_readpage(struct file *file, struct page *page)
{
	printk(KERN_INFO "hw9fs_readpage called\n");
	return block_read_full_page(page, hw9fs_get_block);
}

static const struct address_space_operations hw9fs_aops = {
	.readpage = hw9fs_readpage
};

const struct file_operations hw9fs_fops = {
	.llseek         = generic_file_llseek,
	.read           = do_sync_read,
	.aio_read       = generic_file_aio_read,
};

/* given a directory inode and a dentry, add the relevant dentry->inode mapping to the dentry cache. */
static struct dentry * 
hw9fs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd) {	
	struct inode *inode;
	printk(KERN_INFO "hw9fs_lookup called directory ino %lu, dentry name %s\n", dir->i_ino, dentry->d_name.name);	
	if(memcmp(dentry->d_name.name,"existing_file",13)==0) {
		inode = iget_locked(dir->i_sb,18);
		inode->i_fop = &hw9fs_fops;
		inode->i_size = 1024;
		inode->i_mode = S_IFREG|S_IRWXU|S_IRWXO|S_IRWXG;
		inode->i_mapping->a_ops = &hw9fs_aops;
		d_add(dentry,inode);
	}
	return NULL;
}

/* lists contents of the directory pointed to by filp, one entry at a time (per call).  */
static int hw9fs_readdir(struct file *filp, void *dirent, filldir_t filldir) {
	printk(KERN_INFO "hw9fs_readdir called\n");
	
	
	switch(filp->f_pos) {
	case 0:
		filldir(dirent,"existing_file",13,0,18,DT_REG);
		break;
	case 1:
		filldir(dirent,"nonexisting_file",16,0,19,DT_REG);
		break;
	}
	filp->f_pos++;
	return 0;
}

static int 
hw9fs_fill_super(struct super_block *sb, void *data, int silent);

static struct dentry* 
hw9fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name,
            void *data)
{
	printk(KERN_INFO "Mounting hw9fs %s\n",dev_name);
	return mount_bdev(fs_type, flags, dev_name, data, hw9fs_fill_super);
}

static int 
hw9fs_statfs(struct dentry *dentry, struct kstatfs *buf) {
	printk(KERN_INFO "hw9fs_statfs called\n");
	return 1;
}

/* file system declaration struct used by register_filesystem() */
static struct file_system_type hw9fs = {
    name:        "hw9fs",
    mount:    hw9fs_mount,
    kill_sb:   kill_block_super,
    owner:    THIS_MODULE
};


// operations concerning the "superblock", or the filesystem as a whole
static const struct super_operations hw9fs_sops = {
	.statfs         = hw9fs_statfs,  /* statfs - not used */
	.show_options   = generic_show_options,
};

// operations concerning directories (file = inode + index)
static const struct file_operations hw9fs_dops = {
	.read           = generic_read_dir,
	.readdir        = hw9fs_readdir,
	.llseek         = generic_file_llseek,
};

// operations concerning directory inodes
static const struct inode_operations hw9fs_dir_iops = {
	.lookup         = hw9fs_lookup
};

int init_module(void)
{
	int err;

	printk(KERN_INFO "hw9fs module loaded.\n");
	err = register_filesystem( &hw9fs );
	if(!err) {
		printk(KERN_INFO "hw9fs registered.\n");
	}
	return err;
}

void cleanup_module(void)
{
	int err;
	printk(KERN_INFO "unloading hw9fs\n");	
	err = unregister_filesystem(&hw9fs);
	if(!err) {
		printk(KERN_INFO "hw9fs unregistered.\n");
	}
	else {
		printk(KERN_INFO "hw9fs unregisteration failed.\n");
	}	 
}

struct disk_header {
	char name[26];
	int size;
} ;
	
/* the "super_block" struct holds all the top-level information for the filesystem */
static int
hw9fs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *hw9fs_root_inode;
	struct buffer_head *bh; 

	sb_set_blocksize(sb,1024);
	sb->s_magic = HW9FS_MAGIC;
	sb->s_op = &hw9fs_sops; // super block operations
	sb->s_type = &hw9fs; // file_system_type

	sb->s_fs_info = kmalloc(sizeof(struct disk_header),1);	 	
	if(!(bh = sb_bread(sb, 0))) {
		printk(KERN_INFO "Unable to read superblock\n");
		kfree(sb->s_fs_info);
		goto exit;
	}

	memcpy(sb->s_fs_info,bh->b_data,1024);
	
	// add a silly null-terminator just so we can print something short to the log
	((struct disk_header*)sb->s_fs_info)->dummy[100]=0; 
	printk(KERN_INFO "Disk header starts with: %s", (char*)sb->s_fs_info);

	brelse(bh);
	
	// the root inode will be queried for its contents on "ls", through the readdir f
	hw9fs_root_inode = iget_locked(sb, 1); // allocate an inode with id 1
	hw9fs_root_inode->i_mode = S_IFDIR|S_IRWXU|S_IRWXO|S_IRWXG;  // user, group and other all have RWX privileges
	hw9fs_root_inode->i_op = &hw9fs_dir_iops;
	hw9fs_root_inode->i_fop = &hw9fs_dops;	

	unlock_new_inode(hw9fs_root_inode);
	
	if(!(sb->s_root = d_make_root(hw9fs_root_inode))) {
		iput(hw9fs_root_inode);
		return -ENOMEM;
	}

 exit:
	return 0;
}
