#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/delay.h>

#include <asm/uaccess.h>


MODULE_LICENSE("GPL");

#define SMPLRT_DIV		0x19
#define CONFIG			0x1A
#define GYRO_CONFIG		0x1B
#define ACCEL_CONFIG	0x1C
#define ACCEL_XOUT_H	0x3B
#define ACCEL_XOUT_L	0x3C
#define ACCEL_YOUT_H	0x3D
#define ACCEL_YOUT_L	0x3E
#define ACCEL_ZOUT_H	0x3F
#define ACCEL_ZOUT_L	0x40
#define TEMP_OUT_H		0x41
#define TEMP_OUT_L		0x42
#define GYRO_XOUT_H		0x43
#define GYRO_XOUT_L		0x44
#define GYRO_YOUT_H		0x45
#define GYRO_YOUT_L		0x46
#define GYRO_ZOUT_H		0x47
#define GYRO_ZOUT_L		0x48
#define PWR_MGMT_1		0x6B

#define MPU6050_MAJOR 500
#define MPU6050_MINOR 0

struct mpu6050_device 
{
	struct input_dev *input;
	struct i2c_client *client;
	struct delayed_work work;
};
struct mpu6050_device *mpu6050; 

static int mpu6050_read_byte(struct i2c_client *client, unsigned char reg)
{
	int ret;

	char txbuf[1] = { reg };
	char rxbuf[1];

	struct i2c_msg msg[2] = {
		{client->addr, 0, 1, txbuf},
		{client->addr, I2C_M_RD, 1, rxbuf}
	};

	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0) {
		printk("ret = %d\n", ret);
		return ret;
	}

	return rxbuf[0];
}

static int mpu6050_write_byte(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	char txbuf[2] = {reg, val};

	struct i2c_msg msg[2] = {
		{client->addr, 0, 2, txbuf},
	};

	i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));

	return 0;
}


void mpu6050_work_func(struct work_struct *pwork)
{
	struct mpu6050_device *pdev = (struct mpu6050_device *)(container_of((struct delayed_work *)pwork,struct mpu6050_device,work)); 
	struct i2c_client *client = pdev->client;
	unsigned short gx = 0;
	unsigned short gy = 0;
	unsigned short gz = 0;
	unsigned short ax = 0;
	unsigned short ay = 0;
	unsigned short az = 0;
	unsigned short temp = 0;

	ax = mpu6050_read_byte(client, ACCEL_XOUT_L);
	ax |= mpu6050_read_byte(client, ACCEL_XOUT_H) << 8;
	input_report_abs(pdev->input,ABS_X,ax);

	ay = mpu6050_read_byte(client, ACCEL_YOUT_L);
	ay |= mpu6050_read_byte(client, ACCEL_YOUT_H) << 8;
	input_report_abs(pdev->input,ABS_Y,ay);

	az = mpu6050_read_byte(client, ACCEL_ZOUT_L);
	az |= mpu6050_read_byte(client, ACCEL_ZOUT_H) << 8;
	input_report_abs(pdev->input,ABS_Z,az);

	gx = mpu6050_read_byte(client, GYRO_XOUT_L);
	gx |= mpu6050_read_byte(client, GYRO_XOUT_H) << 8;
	input_report_abs(pdev->input,ABS_RX,gx);

	gy = mpu6050_read_byte(client, GYRO_YOUT_L);
	gy |= mpu6050_read_byte(client, GYRO_YOUT_H) << 8;
	input_report_abs(pdev->input,ABS_RY,gy);

	gz = mpu6050_read_byte(client, GYRO_ZOUT_L);
	gz |= mpu6050_read_byte(client, GYRO_ZOUT_H) << 8;
	input_report_abs(pdev->input,ABS_RZ,gz);

		
	temp = mpu6050_read_byte(client, TEMP_OUT_L);
	temp |= mpu6050_read_byte(client, TEMP_OUT_H) << 8;
	input_report_abs(pdev->input,ABS_MISC,temp);

	input_sync(pdev->input);

	schedule_delayed_work(&pdev->work,msecs_to_jiffies(1000));
}

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	mpu6050 = kzalloc(sizeof(*mpu6050), GFP_KERNEL);
	if (mpu6050 == NULL) 
	{
		printk("kzalloc failed!\n");
		return -ENOMEM;
	}

	mpu6050->client = client;
	
	mpu6050_write_byte(client, PWR_MGMT_1, 0x00);
	mpu6050_write_byte(client, SMPLRT_DIV, 0x07);
	mpu6050_write_byte(client, CONFIG, 0x06);
	mpu6050_write_byte(client, GYRO_CONFIG, 0xF8);
	mpu6050_write_byte(client, ACCEL_CONFIG, 0x19);

	mpu6050->input = input_allocate_device();
	set_bit(EV_ABS,mpu6050->input->evbit);
	input_set_abs_params(mpu6050->input,ABS_X,-32768,32767,0,0);
	input_set_abs_params(mpu6050->input,ABS_Y,-32768,32767,0,0);
	input_set_abs_params(mpu6050->input,ABS_Z,-32768,32767,0,0);
	input_set_abs_params(mpu6050->input,ABS_RX,-32768,32767,0,0);
	input_set_abs_params(mpu6050->input,ABS_RY,-32768,32767,0,0);
	input_set_abs_params(mpu6050->input,ABS_RZ,-32768,32767,0,0);
	input_set_abs_params(mpu6050->input,ABS_MISC,-32768,32767,0,0);
	
	if(input_register_device(mpu6050->input))
	{
		printk("input_register_device failed\n");
		input_free_device(mpu6050->input);
		kfree(mpu6050);
		mpu6050 = NULL;
		return -1;
	}

	INIT_DELAYED_WORK(&mpu6050->work,mpu6050_work_func);
	schedule_delayed_work(&mpu6050->work,msecs_to_jiffies(1000));
	return 0;
}

static int mpu6050_remove(struct i2c_client *client)
{
	cancel_delayed_work(&mpu6050->work);
	input_unregister_device(mpu6050->input);
	input_free_device(mpu6050->input);
	kfree(mpu6050);
	mpu6050 = NULL;

	return 0;
}

static const struct i2c_device_id mpu6050_id[] = {
	{ "mpu6050", 0},
	{}
}; 

static struct of_device_id mpu6050_dt_match[] = {
	{.compatible = "invensense,mpu6050" },
	{/*northing to be done*/},
};

struct i2c_driver mpu6050_driver = {
	.driver = {
		.name 			= "mpu6050",
		.owner 			= THIS_MODULE,
		.of_match_table = of_match_ptr(mpu6050_dt_match),
	},
	.probe 		= mpu6050_probe,
	.remove 	= mpu6050_remove,
	.id_table 	= mpu6050_id,
};

module_i2c_driver(mpu6050_driver);

