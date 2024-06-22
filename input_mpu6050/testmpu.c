#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc,char *argv[])
{
	struct input_event keyevt;
	int fd = -1;

	if(argc < 2)
	{
		printf("The argument is error\n");
		return 1;
	}

	fd = open(argv[1],O_RDONLY);
	if(fd < 0)
	{
		printf("open %s failed\n",argv[1]);
		return 2;
	}

	while(1)
	{
		read(fd,&keyevt,sizeof(keyevt));
		if(keyevt.type == EV_ABS)
		{
			switch(keyevt.code)
			{
				case ABS_X:
					printf("AX-%d\n",keyevt.value);
					break;
				case ABS_Y:
					printf("AY-%d\n",keyevt.value);
					break;
				case ABS_Z:
					printf("AZ-%d\n",keyevt.value);
					break;
				case ABS_RX:
					printf("GX-%d\n",keyevt.value);
					break;
				case ABS_RY:
					printf("GY-%d\n",keyevt.value);
					break;
				case ABS_RZ:
					printf("GZ-%d\n",keyevt.value);
					break;
				case ABS_MISC:
					printf("TEMP-%d\n",keyevt.value);
					break;
			}
		}
	}

	close(fd);
	fd = -1;
	return 0;
}
