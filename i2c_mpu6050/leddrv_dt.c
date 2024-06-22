#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
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

int major = 11;
int minor = 0;
int myled_num  = 1;

struct myled_dev
{
	struct cdev mydev;

	unsigned int led2gpio;
	unsigned int led3gpio;
	unsigned int led4gpio;
	unsigned int led5gpio;
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
			gpio_set_value(pmydev->led2gpio,1);
			break;
		case 3:
			gpio_set_value(pmydev->led3gpio,1);
			break;
		case 4:
			gpio_set_value(pmydev->led4gpio,1);
			break;
		case 5:
			gpio_set_value(pmydev->led5gpio,1);
			break;
	}
}

void led_off(struct myled_dev *pmydev,int ledno)
{
	switch(ledno)
	{
		case 2:
			gpio_set_value(pmydev->led2gpio,0);
			break;
		case 3:
			gpio_set_value(pmydev->led3gpio,0);
			break;
		case 4:
			gpio_set_value(pmydev->led4gpio,0);
			break;
		case 5:
			gpio_set_value(pmydev->led5gpio,0);
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

void request_leds_gpio(struct myled_dev *pmydev,struct device_node *pnode)
{
	pmydev->led2gpio = of_get_named_gpio(pnode,"led2-gpio",0);
	gpio_request(pmydev->led2gpio,"led2");
	
	pmydev->led3gpio = of_get_named_gpio(pnode,"led3-gpio",0);
	gpio_request(pmydev->led3gpio,"led3");
	
	pmydev->led4gpio = of_get_named_gpio(pnode,"led4-gpio",0);
	gpio_request(pmydev->led4gpio,"led4");
	
	pmydev->led5gpio = of_get_named_gpio(pnode,"led5-gpio",0);
	gpio_request(pmydev->led5gpio,"led5");
}

void set_leds_gpio_output(struct myled_dev *pmydev)
{
	gpio_direction_output(pmydev->led2gpio,0);
	gpio_direction_output(pmydev->led3gpio,0);
	gpio_direction_output(pmydev->led4gpio,0);
	gpio_direction_output(pmydev->led5gpio,0);
}

void free_leds_gpio(struct myled_dev *pmydev)
{
	gpio_free(pmydev->led2gpio);
	gpio_free(pmydev->led3gpio);
	gpio_free(pmydev->led4gpio);
	gpio_free(pmydev->led5gpio);
}

int __init myled_init(void)
{
	int ret = 0;
	dev_t devno = MKDEV(major,minor);
	struct device_node *pnode = NULL;
	
	pnode = of_find_node_by_path("/fs4412-leds");
	if(NULL == pnode)
	{
		printk("find node by path failed\n");
		return -1;
	}

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
	request_leds_gpio(pgmydev,pnode);

	/*con-register set output*/
	set_leds_gpio_output(pgmydev);

	return 0;
}

void __exit myled_exit(void)
{
	dev_t devno = MKDEV(major,minor);

	/*iounmap*/
	free_leds_gpio(pgmydev);

	cdev_del(&pgmydev->mydev);

	unregister_chrdev_region(devno,myled_num);

	kfree(pgmydev);
	pgmydev = NULL;
}


MODULE_LICENSE("GPL");

module_init(myled_init);
module_exit(myled_exit);
