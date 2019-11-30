/*****************************************************************************
*
*   利用RAM模拟一个块设备，实现其驱动底层的读写
*   https://linux-kernel-labs.github.io/master/labs/filesystems_part1.html
*
 *****************************************************************************/

#include <linux/fs.h>
#include <linux/module.h>

#define KERN_LOG_LEVEL      KERN_ALERT


#define MYFS_MAGIC          0x858458f6


static const struct super_operations myfs_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};


static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{

    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MYFS_MAGIC ;
    sb->s_op = &myfs_ops;
    
    return 0;
}


struct dentry *myfs_mount(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data)
{
        return mount_nodev(fs_type, flags, data, myfs_fill_super);
}


static struct file_system_type myfs_type = {
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = NULL,
};



static int __init myfs_init(void)
{
    register_filesystem(&myfs_type);

    return 0;
}

static void __exit myfs_exit(void)
{
    unregister_filesystem(&myfs_type);
}


module_init(myfs_init);
module_exit(myfs_exit);

MODULE_LICENSE("GPL v2");
