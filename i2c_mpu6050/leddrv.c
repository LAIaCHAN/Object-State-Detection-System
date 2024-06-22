#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include "leddrv.h"

#define GPX1CON 0x11000C20
#define GPX1DAT 0x11000C24

#define GPX2CON 0x11000C40
#define GPX2DAT 0x11000C44

#define GPF3CON 0x114001E0
#define GPF3DAT 0x114001E4


int major = 11;
int minor = 0;
int myled_num  = 1;

struct myled_dev
{
	struct cdev mydev;

	volatile unsigned long *pled2_con;
	volatile unsigned long *pled2_dat;
	
	volatile unsigned long *pled3_con;
	volatile unsigned long *pled3_dat;

	volatile unsigned long *pled4_con;
	volatile unsigned long *pled4_dat;

	volatile unsigned long *pled5_con;
	volatile unsigned long *pled5_dat;
};

struct myled_dev *pgmydev = NULL;


int myled_open(struct inode *pnode,struct file *pfile)
{
	pfile->private_data =(void *) (container_of(pnode->i_cdev,struct myled_dev,mydev));
	
	return 0;
}

int myled_close(struct inode *pnode,struct file *pfile)
{
	return 0;
}

void led_on(struct myled_dev *pmydev,int ledno)
{
	switch(ledno)
	{
		case 2:
			writel(readl(pmydev->pled2_dat) | (0x1 << 7),pmydev->pled2_dat);
			break;
		case 3:
			writel(readl(pmydev->pled3_dat) | (0x1),pmydev->pled3_dat);
			break;
		case 4:
			writel(readl(pmydev->pled4_dat) | (0x1 << 4),pmydev->pled4_dat);
			break;
		case 5:
			writel(readl(pmydev->pled5_dat) | (0x1 << 5),pmydev->pled5_dat);
			break;
	}
}

void led_off(struct myled_dev *pmydev,int ledno)
{
	switch(ledno)
	{
		case 2:
			writel(readl(pmydev->pled2_dat) & (~(0x1 << 7)),pmydev->pled2_dat);
			break;
		case 3:
			writel(readl(pmydev->pled3_dat) & (~(0x1)),pmydev->pled3_dat);
			break;
		case 4:
			writel(readl(pmydev->pled4_dat) & (~(0x1 << 4)),pmydev->pled4_dat);
			break;
		case 5:
			writel(readl(pmydev->pled5_dat) & (~(0x1 << 5)),pmydev->pled5_dat);
			break;
	}
}


long myled_ioctl(struct file *pfile,unsigned int cmd,unsigned long arg)
{
	struct myled_dev *pmydev = (struct myled_dev *)pfile->private_data;

	if(arg < 2 || arg > 5)
	{
		return -1;
	}
	switch(cmd)
	{
		case MY_LED_ON:
			led_on(pmydev,arg);
			break;
		case MY_LED_OFF:
			led_off(pmydev,arg);
			break;
		default:
			return -1;
	}

	return 0;
}

struct file_operations myops = {
	.owner = THIS_MODULE,
	.open = myled_open,
	.release = myled_close,
	.unlocked_ioctl = myled_ioctl,
};

void ioremap_ledreg(struct myled_dev *pmydev)
{
	pmydev->pled2_con = ioremap(GPX2CON,4);
	pmydev->pled2_dat = ioremap(GPX2DAT,4);

	pmydev->pled3_con = ioremap(GPX1CON,4);
	pmydev->pled3_dat = ioremap(GPX1DAT,4);
	
	pmydev->pled4_con = ioremap(GPF3CON,4);
	pmydev->pled4_dat = ioremap(GPF3DAT,4);
	
	pmydev->pled5_con = pmydev->pled4_con;
	pmydev->pled5_dat = pmydev->pled4_dat;
}

void set_output_ledconreg(struct myled_dev *pmydev)
{
	writel((readl(pmydev->pled2_con) & (~(0xF << 28))) | (0x1 << 28),pmydev->pled2_con);
	writel((readl(pmydev->pled3_con) & (~(0xF))) | (0x1),pmydev->pled3_con);
	writel((readl(pmydev->pled4_con) & (~(0xF << 16))) | (0x1 << 16),pmydev->pled4_con);
	writel((readl(pmydev->pled5_con) & (~(0xF << 20))) | (0x1 << 20),pmydev->pled5_con);

	writel(readl(pmydev->pled2_dat) & (~(0x1 << 7)),pmydev->pled2_dat);
	writel(readl(pmydev->pled3_dat) & (~(0x1)),pmydev->pled3_dat);
	writel(readl(pmydev->pled4_dat) & (~(0x1 << 4)),pmydev->pled4_dat);
	writel(readl(pmydev->pled5_dat) & (~(0x1 << 5)),pmydev->pled5_dat);
}

void iounmap_ledreg(struct myled_dev *pmydev)
{
	iounmap(pmydev->pled2_con);
	pmydev->pled2_con = NULL;
	iounmap(pmydev->pled2_dat);
	pmydev->pled2_dat = NULL;

	iounmap(pmydev->pled3_con);
	pmydev->pled3_con = NULL;
	iounmap(pmydev->pled3_dat);
	pmydev->pled3_dat = NULL;
	
	iounmap(pmydev->pled4_con);
	pmydev->pled4_con = NULL;
	iounmap(pmydev->pled4_dat);
	pmydev->pled4_dat = NULL;
	
	pmydev->pled5_con = NULL;
	pmydev->pled5_dat = NULL;
}

int __init myled_init(void)
{
	int ret = 0;
	dev_t devno = MKDEV(major,minor);

	/*申请设备号*/
	ret = register_chrdev_region(devno,myled_num,"myled");
	if(ret)
	{
		ret = alloc_chrdev_region(&devno,minor,myled_num,"myled");
		if(ret)
		{
			printk("get devno failed\n");
			return -1;
		}
		major = MAJOR(devno);//容易遗漏，注意
	}

	pgmydev = (struct myled_dev *)kmalloc(sizeof(struct myled_dev),GFP_KERNEL);
	if(NULL == pgmydev)
	{
		unregister_chrdev_region(devno,myled_num);
		printk("kmalloc failed\n");
		return -1;
	}
	memset(pgmydev,0,sizeof(struct myled_dev));

	/*给struct cdev对象指定操作函数集*/	
	cdev_init(&pgmydev->mydev,&myops);

	/*将struct cdev对象添加到内核对应的数据结构里*/
	pgmydev->mydev.owner = THIS_MODULE;
	cdev_add(&pgmydev->mydev,devno,myled_num);

	/*ioremap*/
	ioremap_ledreg(pgmydev);

	/*con-register set output*/
	set_output_ledconreg(pgmydev);

	return 0;
}

void __exit myled_exit(void)
{
	dev_t devno = MKDEV(major,minor);

	/*iounmap*/
	iounmap_ledreg(pgmydev);

	cdev_del(&pgmydev->mydev);

	unregister_chrdev_region(devno,myled_num);

	kfree(pgmydev);
	pgmydev = NULL;
}


MODULE_LICENSE("GPL");

module_init(myled_init);
module_exit(myled_exit);
