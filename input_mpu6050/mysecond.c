#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

int major = 11;
int minor = 0;
int mysecond_num = 1;

struct mysecond_dev {
    struct cdev mydev;
    int second;
    struct timer_list timer;
    atomic_t openflag; // 1 can open, 0 can not open
};

struct mysecond_dev gmydev;

// 定时器回调函数
void timer_func(struct timer_list *t) {
//    struct mysecond_dev *pmydev = from_timer(pmydev, t, timer);
	struct mysecond_dev *pmydev = (struct mysecond_dev *)pfile->private_data;
	pmydev->second++;
    mod_timer(&pmydev->timer, jiffies + HZ * 1);
}

int mysecond_open(struct inode *pnode, struct file *pfile) {
    struct mysecond_dev *pmydev = NULL;

    pfile->private_data = (void *)(container_of(pnode->i_cdev, struct mysecond_dev, mydev));
    pmydev = (struct mysecond_dev *)pfile->private_data;

    if (atomic_dec_and_test(&pmydev->openflag)) {
        pmydev->timer.expires = jiffies + HZ * 1;
        timer_setup(&pmydev->timer, timer_func, 0); // 使用新的定时器 API
        add_timer(&pmydev->timer);
        return 0;
    } else {
        atomic_inc(&pmydev->openflag);
        printk("The device is opened already\n");
        return -1;
    }
}

int mysecond_close(struct inode *pnode, struct file *pfile) {
    struct mysecond_dev *pmydev = (struct mysecond_dev *)pfile->private_data;
    del_timer(&pmydev->timer);
    atomic_set(&pmydev->openflag, 1);
    return 0;
}

ssize_t mysecond_read(struct file *pfile, char __user *puser, size_t size, loff_t *p_pos) {
    struct mysecond_dev *pmydev = (struct mysecond_dev *)pfile->private_data; // 确保声明 pmydev
    size = sizeof(int);
    copy_to_user(puser, &pmydev->second, size);
    return size;
}

struct file_operations myops = {
    .owner = THIS_MODULE,
    .open = mysecond_open,
    .read = mysecond_read,
    .release = mysecond_close,
};

int __init mysecond_init(void) {
    int ret = 0;
    dev_t devno = MKDEV(major, minor);

    /* 申请设备号 */
    ret = register_chrdev_region(devno, mysecond_num, "mysecond");
    if (ret) {
        ret = alloc_chrdev_region(&devno, minor, mysecond_num, "mysecond");
        if (ret) {
            printk("get devno failed\n");
            return -1;
        }
        major = MAJOR(devno); // 容易遗漏，注意
    }

    /* 给 struct cdev 对象指定操作函数集 */
    cdev_init(&gmydev.mydev, &myops);

    /* 将 struct cdev 对象添加到内核对应的数据结构里 */
    gmydev.mydev.owner = THIS_MODULE;
    cdev_add(&gmydev.mydev, devno, mysecond_num);

    atomic_set(&gmydev.openflag, 1);
    return 0;
}

void __exit mysecond_exit(void) {
    dev_t devno = MKDEV(major, minor);

    cdev_del(&gmydev.mydev);

    unregister_chrdev_region(devno, mysecond_num);
}

MODULE_LICENSE("GPL");

module_init(mysecond_init);
module_exit(mysecond_exit);

