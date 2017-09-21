#include <linux/module.h>
#include <linux/of_device.h>

static struct platform_device *second_pdev;

static int __init seconddev_init(void)
{
    int ret;
    second_pdev = platform_device_alloc("second_dev", -1);
    if(!second_pdev)
        return -ENOMEM;
    
    ret = platform_device_add(second_pdev);
    if(ret) {
        platform_device_put(second_pdev);
        return ret;
    }

    return 0;
}
module_init(seconddev_init);

static void __exit seconddev_exit(void)
{
    platform_device_unregister(second_pdev);
}
module_exit(seconddev_exit);
MODULE_AUTHOR("Bao hua");
MODULE_LICENSE("GPL v2");