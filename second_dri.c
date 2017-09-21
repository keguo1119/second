#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>
#include <linux/delay.h>

#define SECOND_MAJOR  237
#define USE_MUTEX

static int second_major = SECOND_MAJOR;
module_param(second_major, int, S_IRUGO);

static struct class *second_class;
struct second_dev *second_devp;
static int counter = 0;

struct second_dev {
    struct cdev cdev;
#ifndef  USE_MUTEX
    atomic_t counter;
#else
    int counter;
#endif
    unsigned long state;  // 1 start timer, 0 timer stop
    struct mutex mutex;
    struct timer_list s_timer;
    struct device	*dev;
    struct miscdevice miscdev;
    const struct attribute_group **groups;
    struct work_struct mywork;
};

//static struct second_dev *second_devp_; 

static void second_timer_handler(unsigned long arg)
{
    struct second_dev *dev = (void *) arg;
    mod_timer(&dev->s_timer, jiffies + HZ);
#ifndef  USE_MUTEX
     atomic_inc(&dev->counter);
#else
    printk(KERN_INFO "%s mutex lock\n", __func__);
    mutex_lock(&dev->mutex);
    dev->counter++;
    mutex_unlock(&dev->mutex);
#endif
//    printk(KERN_INFO "current jiffies is %ld\n", jiffies);
}

static void second_timer_ctl(struct work_struct *ws)
{
    struct second_dev *dev =
		container_of(ws, struct second_dev, mywork);
	int ret = 0;

    printk(KERN_INFO "%s trigger_store test\n", __func__);
    mdelay(500);
    return;
}

static int second_open(struct inode *inode, struct file *filp)
{
    mutex_lock(&second_devp->mutex);
    if(second_devp->state == 0)
    {
       setup_timer(&second_devp->s_timer, second_timer_handler, (unsigned long)second_devp);
       add_timer(&second_devp->s_timer);
       second_devp->state = 1;
    }
    mutex_unlock(&second_devp->mutex);
 
//    filp->private_data = second_devp;
    printk(KERN_INFO "%s-open 1\n", __func__);
    return 0;
}

static int second_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t second_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int counter;
//    struct second_dev *second_devp = filp->private_data;

//    struct second_dev *second_devp = container_of(filp->private_data, 
//        struct second_dev, miscdev);  
#ifndef  USE_MUTEX
    counter = atomic_read(&second_devp->counter);
#else
    printk(KERN_INFO "%s mutex lock\n", __func__);
    mutex_lock(&second_devp->mutex);
    counter = second_devp->counter;
    mutex_unlock(&second_devp->mutex);
#endif
    if(put_user(counter, (int *)buf))
        return -EFAULT;
    else
        return sizeof(unsigned int);
}

static ssize_t second_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *ppos)
{
    return 0;
}

ssize_t trigger_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
    struct second_dev *data = dev_get_drvdata(dev);
    unsigned long state = 0;
    ssize_t			status;
    int ret;

    printk(KERN_INFO "%s: 0 - mutex lock\n", __func__);
    mutex_lock(&data->mutex);
    ret = kstrtoul(buf, 10, &state);
    if (ret)
        goto unlock;
    
    if (state == 1)
    {
        printk(KERN_INFO "%s: 1 - mutex lock, state = %ld\n", __func__, state);
        if (data->state == 0)
        {
            setup_timer(&second_devp->s_timer, second_timer_handler, (unsigned long)second_devp);
            add_timer(&data->s_timer);
            data->state = 1;
        }
    }
    else
    {
        printk(KERN_INFO "%s: 2 - mutex lock, state = %ld\n", __func__, state);
        if(data->state == 1)
        {
            del_timer(&data->s_timer);
            data->state = 0;
        }
    }

    printk(KERN_INFO "%s: 1 - mutex lock, state = %d\n", __func__, state);
unlock:
    mutex_unlock(&data->mutex);

    return status ? : size;
}

ssize_t trigger_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
    int cnt = -1;

    struct second_dev *sdev = dev_get_drvdata(dev);

#ifndef  USE_MUTEX
    cnt = atomic_read(&sdev->counter);
#else
    printk(KERN_INFO "%s mutex lock\n", __func__);
    mutex_lock(&sdev->mutex);
//    mutex_lock_interruptible(&second_devp->mutex);
    cnt = sdev->counter;
    mutex_unlock(&sdev->mutex);
#endif   

     return sprintf(buf, "second_trigger_show: count =%d\n", cnt);
//    return sprintf(buf, "second_trigger_show: count = %d, counter_addr=%p\n", *counter, counter);
}
static DEVICE_ATTR_RW(trigger);
//static DEVICE_ATTR(trigger, 0644, trigger_show, trigger_store);
static struct attribute *second_trigger_attrs[] = {
	&dev_attr_trigger.attr,
	NULL,
};
static const struct attribute_group second_trigger_group = {
	.attrs = second_trigger_attrs,
};

static const struct attribute_group *second_groups[] = {
    &second_trigger_group,
    NULL,
};

static const struct file_operations second_fops = {
    .owner  = THIS_MODULE,
    .open   = second_open,
    .release= second_release,
    .read   = second_read,
    .write  = second_write,
};

static int second_probe(struct platform_device *pdev)
{
    int ret, err;
    struct device *dev = &pdev->dev;
    dev_t devno = MKDEV(SECOND_MAJOR, 0);

    second_devp = devm_kzalloc(dev, sizeof(*second_devp), GFP_KERNEL);
    if(!second_devp)
    {
        dev_info(&pdev->dev, "second_devp  devm_kzalloc failed\n");
        return -ENOMEM;
    }

#ifndef  USE_MUTEX
   atomic_set(&second_devp->counter, 0);
#else
    printk(KERN_INFO "%s mutex lock\n", __func__);
    mutex_init(&second_devp->mutex);
    mutex_lock(&second_devp->mutex);
    second_devp->counter = 0;
    second_devp->state = 0;
    mutex_unlock(&second_devp->mutex); 
#endif

    INIT_WORK(&second_devp->mywork, second_timer_ctl);

    cdev_init(&second_devp->cdev, &second_fops);
    second_devp->cdev.owner = THIS_MODULE;
    err = cdev_add(&second_devp->cdev, devno, 1);
    if(err)
    {
         printk(KERN_DEBUG "Error %d adding second\n", err);
         goto fail_malloc;
    }
       
    second_devp->dev = device_create(second_class,  dev,
                    MKDEV(SECOND_MAJOR, 0), second_devp, "second");	
                    
//    second_devp->dev =	device_create_with_groups(second_class,  dev,  
//        MKDEV(SECOND_MAJOR, 0), second_devp, second_devp->groups,"second");
//    platform_set_drvdata(pdev, second_devp);

    dev_info(&pdev->dev, "second_dev drv proded\n");
    return 0;

fail_malloc:
	printk(KERN_INFO "6 - probe failed\n");
    return ret;        
}


static int second_remove(struct platform_device *pdev)
{
    device_destroy(second_class, MKDEV(SECOND_MAJOR, 0));
    return 0;
}

static struct platform_driver second_driver = {
    .driver = {
        .name  = "second_dev",
        .owner = THIS_MODULE, 
    },
    .probe  = second_probe,
    .remove = second_remove,
};

static int __init second_init(void)
{
	int ret;

    second_class = class_create(THIS_MODULE, "second");
    if (IS_ERR(second_class))
		return PTR_ERR(second_class);
    second_class->dev_groups = second_groups;

	ret = platform_driver_register(&second_driver);
	if (ret)
		printk(KERN_ERR "second: probe failed: %d\n", ret);

	return ret;
}
module_init(second_init);

static void __exit second_exit(void)
{
    if(second_devp)
    {
        cdev_del(&second_devp->cdev);
        if(second_devp->state == 1)
            del_timer(&second_devp->s_timer);   
    }
      
    device_destroy(second_class, MKDEV(SECOND_MAJOR, 0));
    class_destroy(second_class);

    platform_driver_unregister(&second_driver);
    printk(KERN_INFO "second: exit\n");
}
module_exit(second_exit);


MODULE_AUTHOR("Bao hua");
MODULE_LICENSE("GPL v2");