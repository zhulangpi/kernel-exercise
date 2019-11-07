/*****************************************************************************
*
*   利用RAM模拟一个块设备，实现其驱动底层的读写
*   https://linux-kernel-labs.github.io/master/labs/block_device_drivers.html
*
 *****************************************************************************/

#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/module.h>

#define KERN_LOG_LEVEL      KERN_ALERT

#define MY_BLKDEV_MAJOR             240
#define MY_BLKDEV_MINOR             1
#define MY_BLKDEV_NAME              "my_blkdev"

#define NR_SECTORS                  1024

#define KERNEL_SECTOR_SIZE          512


struct my_blkdev{
    spinlock_t      lock;
    struct gendisk  *gd;
    struct request_queue   *queue;
    size_t          size;
    char            *data;          //对应实际的存储介质
};

static struct my_blkdev *dev;

static int my_blkdev_open(struct block_device *bdev, fmode_t mode)
{
    printk(KERN_LOG_LEVEL "open my blkdev\n");
    return 0;
}

static void my_blkdev_release(struct gendisk *gd, fmode_t mode)
{
    printk(KERN_LOG_LEVEL "release my blkdev\n");
}

struct block_device_operations my_blkdev_ops = {
    .owner = THIS_MODULE,
    .open = my_blkdev_open,
    .release = my_blkdev_release,
};

//用于处理设备请求
static void my_blkdev_handle_request(struct request_queue *rq_q)
{
    struct request *rq = NULL;
    unsigned long offset = 0, len = 0;
    struct bio_vec bvec;
    struct req_iterator iter;
    char *page_addr=NULL;

    printk( KERN_LOG_LEVEL "handle request\n");

    //遍历request_queue中的每个request
    while( (rq = blk_fetch_request(rq_q)) ){

        if (blk_rq_is_passthrough(rq)) {
            printk(KERN_NOTICE "Skip non-fs request\n");
            __blk_end_request_all(rq, -EIO);
            continue;
        }

        printk(KERN_LOG_LEVEL "request received: pos=%llu bytes=%u cur_bytes=%u dir=%c\n",  (unsigned long long) blk_rq_pos(rq), blk_rq_bytes(rq), blk_rq_cur_bytes(rq), rq_data_dir(rq) ? 'W' : 'R');

        if(offset + len > dev->size){
            __blk_end_request_all(rq, 0);
            continue;
        }

        //遍历queue中的每个bio
        rq_for_each_segment(bvec, rq, iter) {
            offset = iter.iter.bi_sector * KERNEL_SECTOR_SIZE;
            len = iter.iter.bi_size;

            if(offset + len > dev->size){
                continue;
            }

            page_addr = kmap_atomic(bvec.bv_page);
            
            if(rq_data_dir(rq)){    //write
                memcpy( dev->data + offset , page_addr + bvec.bv_offset, bvec.bv_len);
            }else{
                memcpy( page_addr + bvec.bv_offset,  dev->data + offset, len);
            }
            kunmap_atomic(page_addr);
        }

        __blk_end_request_all(rq, 0);

    }
}


/*
    1. 关联 设备号 与 相应数据结构
        dev_t -> gendisk.major -> gendisk.fops
*/

static int __init my_blkdev_init(void)
{
    int status,ret=-1;

    dev = (struct my_blkdev*)vmalloc( sizeof(struct my_blkdev) );

    if(dev==NULL)
        return -ENOMEM;

    dev->size = KERNEL_SECTOR_SIZE * NR_SECTORS;
    dev->data = (char*)vmalloc(dev->size);

    if(dev->data == NULL){
        ret = -ENOMEM;
        goto nomem;
    }

    //1. 注册该设备号的设备
    status = register_blkdev(MY_BLKDEV_MAJOR, MY_BLKDEV_NAME);
    if (status < 0) {
             printk(KERN_ERR "unable to register mybdev block device\n");
             return -EBUSY;
     }
     
    //2. 分配gendisk数据结构并填充基本数据
    dev->gd = alloc_disk(MY_BLKDEV_MINOR);   //几个次设备号，也相当于磁盘几个分区

    dev->gd->major = MY_BLKDEV_MAJOR;
    dev->gd->first_minor = 0;   //第一个次设备号

    dev->gd->private_data = dev;
    snprintf(dev->gd->disk_name, DISK_NAME_LEN, "my_blkdev");
    set_capacity(dev->gd, NR_SECTORS);

    //3. 绑定操作集
    dev->gd->fops = &my_blkdev_ops;

    //4. 初始化请求队列并绑定自己的请求处理函数
    spin_lock_init(&dev->lock);

    dev->queue = blk_init_queue(my_blkdev_handle_request, &dev->lock);
    dev->gd->queue = dev->queue;
    if(dev->queue == NULL)
        goto out_err;

    blk_queue_logical_block_size(dev->queue, KERNEL_SECTOR_SIZE);
    dev->queue->queuedata = dev;


    add_disk(dev->gd);

    return 0;

    blk_cleanup_queue(dev->gd->queue);
out_err:
    unregister_blkdev(MY_BLKDEV_MAJOR, MY_BLKDEV_NAME);
    del_gendisk(dev->gd);
nomem:
    vfree(dev);

    return ret;
}

static void __exit my_blkdev_exit(void)
{

    if(dev->gd)
        del_gendisk(dev->gd);

    blk_cleanup_queue(dev->gd->queue);
    unregister_blkdev(MY_BLKDEV_MAJOR, MY_BLKDEV_NAME);
    vfree(dev);
}


module_init(my_blkdev_init);
module_exit(my_blkdev_exit);

MODULE_LICENSE("GPL v2");
