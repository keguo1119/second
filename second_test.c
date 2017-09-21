#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>

#define SECOND_MAJOR  249

static int second_major = SECOND_MAJOR;
module_param(second_major, int, S_IRUGO);

static struct class *second_class;

struct second_dev {
    struct cdev cdev;
    atomic_t counter;
    struct timer_list s_timer;
    struct device	*dev;
};

static struct second_dev *second_devp; 

static void second_timer_handler(unsigned long arg)
{
    struct second_dev *second_devp = (void *) arg;
    mod_timer(&second_devp->s_timer, jiffies + HZ);
    atomic_inc(&second_devp->counter);

    printk(KERN_INFO "current jiffies is %ld\n", jiffies);
}

static int second_open(struct inode *inode, struct file *filp)
{
 /*   init_timer(&second_devp->s_timer);
    second_devp->s_timer.function   = &second_timer_handler;
    second_devp->s_timer.expires    = jiffies + HZ;
    add_timer(&second_devp->s_timer);
*/
 //   printk(KERN_INFO "%s: 0- second_devp=NULL\n", __func__);
 //   struct second_dev *second_devp = filp->private_data;

    setup_timer(&second_devp->s_timer, second_timer_handler, (unsigned long)second_devp);
    add_timer(&second_devp->s_timer);
    atomic_set(&second_devp->counter, 0);

    return 0;
 //   struct second_dev
}

static int second_release(struct inode *inode, struct file *filp)
{
 //   struct second_dev *second_devp = filp->private_data;
    del_timer(&second_devp->s_timer);

    return 0;
}

static ssize_t second_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int counter;
 //   struct second_dev *second_devp = filp->private_data;
    counter = atomic_read(&second_devp->counter);

    if(put_user(counter, (int *)buf))
        return -EFAULT;
    else
        return sizeof(unsigned int);
}

static const struct file_operations second_fops = {
    .owner  = THIS_MODULE,
    .open   = second_open,
    .release= second_release,
    .read   = second_read,
};


ssize_t second_trigger_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
    return 0;
/*
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	unsigned long state;
	ssize_t ret;

	mutex_lock(&led_cdev->led_access);

	if (led_sysfs_is_disabled(led_cdev)) {
		ret = -EBUSY;
		goto unlock;
	}

	ret = kstrtoul(buf, 10, &state);
	if (ret)
		goto unlock;

	if (state == LED_OFF)
		led_trigger_remove(led_cdev);
	led_set_brightness(led_cdev, state);

	ret = size;
unlock:
	mutex_unlock(&led_cdev->led_access);
	return ret;
    */
}


ssize_t second_trigger_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
//	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
//	led_update_brightness(led_cdev);

//	return sprintf(buf, "%u\n", led_cdev->brightness);
    return sprintf(buf, "second_trigger_show");
}

static DEVICE_ATTR(trigger, 0644, second_trigger_show, second_trigger_store);
static struct attribute *second_trigger_attrs[] = {
	&dev_attr_trigger.attr,
	NULL,
};
static const struct attribute_group second_trigger_group = {
	.attrs = second_trigger_attrs,
};

static const struct attribute_group *second_groups[] = {
	&second_trigger_group,
};

static void second_setup_cdev(struct second_dev *dev, int index)
{
    int err, devno = MKDEV(second_major, index);

    cdev_init(&dev->cdev, &second_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);

    if(err)
        printk(KERN_NOTICE "Error %d adding second%d", err, index);
}

static int __init second_init(void )
{
    int ret;
    dev_t devno = MKDEV(second_major, 0);
//    struct second_dev *second_devp; 

    if(second_major)
        ret = register_chrdev_region(devno, 1, "second_test");
    else {
         ret = alloc_chrdev_region(&devno, 0, 1, "second_test");
         second_major = MAJOR(devno);
    }
    if(ret < 0)
        return ret;
    
//    second_devp = kzalloc(sizeof(struct second_dev), GFP_KERNEL);
    second_devp = devm_kzalloc(second_devp->dev, sizeof(struct second_dev), GFP_KERNEL);
    if(!second_devp) {
        ret = -ENOMEM;
        goto fail_malloc;
    }

    printk(KERN_INFO "insmod second_test\n");

    second_setup_cdev(second_devp, 0);

    second_class = class_create(THIS_MODULE, "second");
	if (IS_ERR(second_class))
		return PTR_ERR(second_class);
//	second_class->pm = &second_class_dev_pm_ops;
	second_class->dev_groups = second_groups;

    printk(KERN_INFO "try device_create_with_groups !\n");

    second_devp->dev = device_create_with_groups(second_class, second_devp->dev, 0, 
        second_devp, second_groups, "second%d", 0);

    if (IS_ERR(&second_devp->dev))
    {
        printk(KERN_INFO "device_create_with_groups failed\n");
        return PTR_ERR(second_devp->dev);
    }
		

 //   led_cdev->dev = device_create_with_groups(leds_class, parent, 0,
 //               led_cdev, led_cdev->groups,
 //               "%s", led_cdev->name);

    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}
module_init(second_init);

static void __exit second_exit(void)
{
 //   cdev_del(&second_devp->cdev);
 //   kfree(second_devp);
    unregister_chrdev_region(MKDEV(second_major, 0), 1);
}
module_exit(second_exit);

MODULE_AUTHOR("Bao hua");
MODULE_LICENSE("GPL v2");