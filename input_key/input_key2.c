#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <asm/uaccess.h>


struct pin_desc
{
	int irq;
	char *name;
	int gpio;
	unsigned int key_val;
};

struct mykey
{
	struct input_dev *pdev;
	struct pin_desc keyinfo;
	struct device_node *pnode;
};

struct mykey *pgkey = NULL;

irqreturn_t key2_irq_handle(int no,void *arg)
{
	struct mykey *pmykey = (struct mykey *)arg;
	int status1 = 0;
	int status2 = 0;

	status1 = gpio_get_value(pmykey->keyinfo.gpio);
	mdelay(1);
	status2 = gpio_get_value(pmykey->keyinfo.gpio);

	if(status1 != status2)
	{
		return IRQ_NONE;
	}

	if(status1)
	{
		input_event(pmykey->pdev,EV_KEY,pmykey->keyinfo.key_val,0);
		input_sync(pmykey->pdev);
	}
	else
	{
		input_event(pmykey->pdev,EV_KEY,pmykey->keyinfo.key_val,1);
		input_sync(pmykey->pdev);
	}
	return IRQ_HANDLED;
}

int __init fs4412key2_init(void)
{
	int ret = 0;

	pgkey = kzalloc(sizeof(struct mykey),GFP_KERNEL);
	if(NULL == pgkey)
	{
		printk("kzalloc failed\n");
		return -1;
	}


	pgkey->pnode = of_find_node_by_path("/mykey2_node");
	if(NULL == pgkey->pnode)
	{
		printk("find node failed\n");
		return -1;
	}


	pgkey->keyinfo.gpio = of_get_named_gpio(pgkey->pnode,"key2-gpio",0);

	pgkey->keyinfo.irq = irq_of_parse_and_map(pgkey->pnode,0);

	pgkey->keyinfo.name = "key2";

	pgkey->keyinfo.key_val = KEY_2;

	pgkey->pdev = input_allocate_device();

	set_bit(EV_KEY,pgkey->pdev->evbit);
	set_bit(pgkey->keyinfo.key_val,pgkey->pdev->keybit);

	ret = input_register_device(pgkey->pdev);

	ret += request_irq(pgkey->keyinfo.irq,
			          key2_irq_handle,
					  IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
					  pgkey->keyinfo.name,
					  pgkey);
	if(ret)
	{
		printk("request_irq failed\n");
		input_unregister_device(pgkey->pdev);
		input_free_device(pgkey->pdev);
		kfree(pgkey);
		pgkey = NULL;
		return -1;
	}
	return 0;
}

void __exit fs4412key2_exit(void)
{
	input_unregister_device(pgkey->pdev);
	input_free_device(pgkey->pdev);

	kfree(pgkey);
	pgkey = NULL;
}


MODULE_LICENSE("GPL");

module_init(fs4412key2_init);
module_exit(fs4412key2_exit);

