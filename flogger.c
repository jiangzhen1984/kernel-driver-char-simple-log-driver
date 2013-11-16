
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mutex.h>


#define DEVICE_NAME "flogger"

#define BUFFER_SIZE 1024 * 1024
#define MAX_CON_COUNT 1

#define USED (1)
#define UNUSED (0)


struct flogger {
    struct device * p_device;
    struct cdev c_dev;
    char buf[BUFFER_SIZE];
    unsigned int pos;
    struct mutex w_lock;
};



struct logger_dev {
    struct flogger * logger;
    unsigned int offset;
    int flag;
} list[MAX_CON_COUNT];




struct flogger * g_logger;
struct class * c_p;
static dev_t dev;


static int flogger_open (struct inode * inode, struct file *file)
{
   int i;
   struct logger_dev * ld; 
    printk("===========do open\n");
    for(i = 0; i < MAX_CON_COUNT; i++) {
        if ((list[i].flag & USED) == USED) {
            continue;
        }
        ld = & list[i];
        ld->logger = g_logger;
        ld->offset = 0;
        ld->flag |= USED;
        file->private_data = ld;
        break;
    }

    if (i >= MAX_CON_COUNT) {
        printk("===== max count\n");
        return -EBUSY;
    } else {
        return  0;
    }
}



static int flogger_release(struct inode * node, struct file * file)
{
    struct logger_dev * ld = file->private_data;
    ld->logger = NULL;
    ld->offset = 0;
    ld->flag &= UNUSED; 
    return 0;
}


static ssize_t flogger_read(struct file * file, char __user * buf,
                      size_t count, loff_t * pos)
{
    return 0;
}

static ssize_t flogger_write(struct file * file, char __user * buf,
                      size_t count, loff_t * pos)
{
    return 0;
}


static const struct file_operations flogger_ops = {
    .owner = THIS_MODULE,
    .open = flogger_open,
    .release = flogger_release,
    .read = flogger_read,
    .write = flogger_write,
};


static int __init flogger_init(void)
{
    int r;
    g_logger = kmalloc(sizeof(struct flogger), GFP_KERNEL);
    if (!g_logger) {
        pr_err(" not avail memory\n");
        return -ENOMEM;
    }


    r = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (r) {
        printk("can't not alloc dev  %d\n", r);
        goto error_bail;
    }


    c_p = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(c_p)) {
        printk("can't not create class\n");
        goto clean_char;
    }
    

    g_logger->pos = 0;
    g_logger->c_dev.owner = THIS_MODULE;

    cdev_init(&g_logger->c_dev, &flogger_ops);
    mutex_init(&g_logger->w_lock);

    r = cdev_add(&g_logger->c_dev, MKDEV(MAJOR(dev), 0), 1);
    if (r) {
        printk(" can't not do cdev_add  %d\n", r);
        goto clean_class;
    }

    g_logger->p_device = device_create(c_p, NULL, MKDEV(MAJOR(dev), 0), 0, DEVICE_NAME);    
    if (IS_ERR(g_logger->p_device)) {
        printk(" can't not create device\n");
        goto clean_dev;
    }

    return r;
clean_dev:
    cdev_del(&g_logger->c_dev);
    mutex_destroy(&g_logger->w_lock);
clean_class:
    class_destroy(c_p);
clean_char:
    unregister_chrdev_region(dev, 1);
error_bail:
    if (g_logger) {
        kfree(g_logger);
    }
    return r;
}



static void __exit flogger_exit(void)
{

    mutex_destroy(&g_logger->w_lock);
    device_destroy(c_p, MKDEV(MAJOR(dev), 0));
    cdev_del(&g_logger->c_dev);
    class_destroy(c_p);
    unregister_chrdev_region(dev, 1);

    if (g_logger) {
        printk("-------------release memory %d\n", g_logger);
        kfree(g_logger);
    }

}





module_init(flogger_init);
module_exit(flogger_exit);

MODULE_LICENSE("GPL v2");


