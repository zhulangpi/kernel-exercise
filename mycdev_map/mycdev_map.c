/*****************************************************************************
*
*   实现一个字符设备驱动的mmap函数，具体的物理页用vmalloc()进行申请
*
*
*   参考 陈莉君 《LINUX内核分析与应用》 动手实践4.5
*   https://github.com/ljrkernel/linuxmooc
*
 *****************************************************************************/

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/vmalloc.h>


#define CDEV_MAJOR   120
#define CDEV_NAME    "mapnopage"
#define MAP_PAGE_COUNT  10
#define MAP_LEN          (PAGE_SIZE*MAP_PAGE_COUNT)


static char *map_area = NULL;


static int mycdev_mmap(struct file *, struct vm_area_struct *);
vm_fault_t map_fault(struct vm_fault *vmf);


//该操作集在注册cdev时被绑定
static struct file_operations cdev_fops = {
    .owner = THIS_MODULE,
    .mmap = mycdev_mmap,
};


//该操作集在mmap调用时绑定
static struct vm_operations_struct map_vm_ops = {
    .fault = map_fault,
};


//在用户态对cdev执行mmap时调用，对map_area进行map
static int mycdev_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;

    if(size > MAP_LEN){
        return -ENXIO;  /* No such device or address */
    }

    //只考虑一个mmap内存区，所以写只支持共享映射。读则无所谓
    if( (vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED) ){
        return -EINVAL; /* Invalid argument */
    }

    //为方便，只允许以0为偏移映射
    if(offset == 0){
        vma->vm_ops = &map_vm_ops;
    }else{
        return -EINVAL;
    }

    return 0;
}


//用户态映射的虚拟地址区间发生缺页时调用
vm_fault_t map_fault(struct vm_fault *vmf)
{
    unsigned long offset = vmf->address - vmf->vma->vm_start;
    void* map_addr = map_area + offset;
    struct page* page = NULL;

//1. 分配具体页给缺的页
    page = vmalloc_to_page(map_addr);
    get_page(page);
    vmf->page = page;

    return 0;
}


static int __init mycdev_init(void)
{
    int ret = 0;

//1.初始化字符设备描述符cdev并添加进内核
    ret = register_chrdev(CDEV_MAJOR, CDEV_NAME, &cdev_fops);
    if(ret<0){
        printk("register failed\n");
        return ret;
    }

//2.为cdev分配内存用于mmap
    map_area = (char*)vmalloc(MAP_LEN);
    if(map_area==NULL){
        printk("vmalloc failed\n");
        return -ENOSPC; /* No space left on device */
    }

    printk("init complete\n");

    return 0;
}


static void __exit mycdev_exit(void)
{

//1.释放内存
    if(map_area!=NULL)
        vfree(map_area);

//2.注销cdev
    unregister_chrdev(CDEV_MAJOR, CDEV_NAME);

    printk("exit complete\n");

}


module_init(mycdev_init);
module_exit(mycdev_exit);
MODULE_LICENSE("GPL");
